"""
The functions translated from the retail program (asmc.py) as a part of their own, built where the
player's disc is: the port's build gives them no bodies, only the signatures the disc gave them
(the "interface", a file kept with the port), and calls each through a pointer that the part built
from the disc fills when the game starts. An installed launcher with no compiler of the port's own
(Android) builds that part from the player's image (asmdev.py) and the game loads it.

What the interface keeps: for every function whose signature or place the assembly stage changed,
its signature after it, and whether it was translated (unit "<asm>"); the static functions' too.
Names, addresses and C types only: nothing of the retail program's bytes.
"""

from __future__ import annotations

import json
import re
from pathlib import Path

import ctype
from program import Function, Program, Signature, cname, prototype

FORMAT = 2
# The unit of a function whose body the part built from the disc has (asmext.trampoline).
EXTERNAL = "<asm-ext>"


def type_json(t):
    if isinstance(t, ctype.Base):
        return {"b": t.name, "q": sorted(t.quals)}
    if isinstance(t, ctype.Ptr):
        return {"p": type_json(t.to), "q": sorted(t.quals)}
    if isinstance(t, ctype.Arr):
        return {"a": type_json(t.of), "n": t.size, "vla": t.vla}
    if isinstance(t, ctype.Func):
        return {"f": type_json(t.ret), "ps": [type_json(p) for p in t.params], "v": t.variadic, "pr": t.prototyped}
    raise TypeError(f"no form for {t!r}")


def type_of(j):
    if "b" in j:
        return ctype.Base(j["b"], frozenset(j["q"]))
    if "p" in j:
        return ctype.Ptr(type_of(j["p"]), frozenset(j["q"]))
    if "a" in j:
        return ctype.Arr(type_of(j["a"]), j["n"], j["vla"])
    return ctype.Func(type_of(j["f"]), tuple(type_of(p) for p in j["ps"]), j["v"], j["pr"])


def sig_json(s: Signature):
    return type_json(ctype.Func(s.ret, tuple(s.params), s.variadic, s.prototyped))


def sig_of(j) -> Signature:
    return Signature.of(type_of(j))


def snapshot(program: Program) -> dict:
    """What the assembly stage may change, before it runs."""
    return {"functions": {k: (f.sig, f.unit, f.inherits) for k, f in program.functions.items()},
            "statics": {k: (f.sig, f.inherits) for k, f in program.statics.items()}}


def interface(program: Program, before: dict) -> dict:
    """The assembly stage's changes to PROGRAM since BEFORE (snapshot), as the interface file holds them."""
    functions = {}
    for k, f in sorted(program.functions.items()):
        was = before["functions"].get(k)
        translated = f.unit == "<asm>"
        if was is None or was[0] != f.sig or was[2] != f.inherits or translated:
            functions[k] = {"sig": sig_json(f.sig), "asm": translated, "new": was is None, "inherits": f.inherits}
    statics = [[unit, name, sig_json(f.sig), f.inherits] for (unit, name), f in sorted(program.statics.items())
               if before["statics"].get((unit, name)) != (f.sig, f.inherits)]
    return {"format": FORMAT, "functions": functions, "statics": statics}


def write(path: Path, data: dict) -> None:
    text = json.dumps(data, indent=1, sort_keys=True) + "\n"
    if not path.exists() or path.read_text() != text:
        path.write_text(text)


def read(path: Path) -> dict:
    data = json.loads(path.read_text())
    if data.get("format") != FORMAT:
        raise ValueError(f"{path}: interface format {data.get('format')}, this hostgen reads {FORMAT}")
    return data


def apply(program: Program, data: dict) -> list[str]:
    """The interface's signatures given to PROGRAM, as the assembly stage would have, and its
    translated functions placed outside the build (EXTERNAL). Returns those functions' names."""
    external = []
    for k, e in data["functions"].items():
        f = program.functions.get(k)
        if f is None:
            f = program.functions[k] = Function(k, sig_of(e["sig"]), None, asm=True)
        f.sig = sig_of(e["sig"])
        f.inherits = e["inherits"]
        if e["asm"]:
            f.unit = EXTERNAL
            external.append(k)
    for unit, name, s, inherits in data["statics"]:
        f = program.statics.get((unit, name))
        if f is not None:
            f.sig = sig_of(s)
            f.inherits = inherits
    program.index_places()
    return sorted(external)


def differences(a: dict, b: dict) -> list[str]:
    """Where interfaces A (the port's) and B (made from a disc now) differ, by name."""
    out = []
    for k in sorted(set(a["functions"]) | set(b["functions"])):
        if a["functions"].get(k) != b["functions"].get(k):
            out.append(k)
    if a["statics"] != b["statics"]:
        out.append("(static functions)")
    return out


def slot(sym: str) -> str:
    return f"openrac_asmx_{cname(sym)}"


def stale_exports(text: str, external: list[str], program: Program) -> list[str]:
    """Where a Windows program's list of exports for the part (tools/asm_part.py exports, written
    from the disc) no longer fits PROGRAM built without the disc, whose EXTERNAL functions the part
    fills: a slot the part fills that the list leaves out or one the program no longer has, and a
    function the list exports that the program no longer has. Either stops the Windows link of the
    program or of the part. Empty when the list fits."""
    entries = {line.split()[0] for line in text.splitlines()[1:] if line.strip()}
    listed = {e for e in entries if e.startswith("openrac_asmx_")}
    slots = {slot(s) for s in external}
    functions = {cname(s) for s in program.functions}
    out = [f"{e}: the part fills it, the list leaves it out" for e in sorted(slots - listed)]
    out += [f"{e}: no such slot in the program" for e in sorted(listed - slots)]
    # A game function by its C name: func_..., or game_... for the others (cname).
    out += [f"{e}: no such function in the program" for e in sorted(entries)
            if e.startswith(("func_", "game_")) and e not in functions]
    return out


def own_name(sym: str) -> str:
    return f"openrac_asmd_{cname(sym)}"


def defined_name(program: Program, sym: str) -> str:
    """The C name a translation is defined under: NAME__game for a function the port wraps
    (a "wrap" row: its host C is NAME and calls NAME__game)."""
    return f"{cname(sym)}__game" if sym in program.wrap else cname(sym)


def trampoline(f: Function, typedefs=None, wrapped: bool = False) -> str:
    """The port's build's body for an EXTERNAL function: a call through its slot, which the part
    built from the disc fills (register_text)."""
    proto = prototype(f, typedefs)
    if proto is None or f.sig.variadic:
        raise ValueError(f"{f.symbol}: no call through a pointer for its signature")
    if not f.sig.prototyped:
        # Declared with no parameter list; its translation takes none (asmc writes it "(void)").
        proto = proto[:-2] + "(void)"
    head, params = proto.split("(", 1)
    params = params[:-1]
    names = [] if params == "void" else [p.rsplit(" ", 1)[1] for p in params.split(", ")]
    types = "void" if params == "void" else ", ".join(p.rsplit(" ", 1)[0] for p in params.split(", "))
    ret = head[: -len(cname(f.symbol))].strip()
    if wrapped:
        proto = proto.replace(f" {cname(f.symbol)}(", f" {cname(f.symbol)}__game(", 1)
    call = f"(({ret} (*)({types}))openrac_asm_external({slot(f.symbol)}, \"{f.symbol}\"))({', '.join(names)})"
    body = f"{call};" if ret == "void" else f"return {call};"
    return f"void *{slot(f.symbol)};\n{proto} {{\n    {body}\n}}\n"


def renamed(sym: str, text: str, defined: str | None = None) -> str:
    """A translation's C with its definition under the part's own name, so it does not stand in
    for the port's trampoline: calls to it still go through the slot."""
    lines = text.splitlines()
    head = next(k for k, l in enumerate(lines) if l.rstrip().endswith("{") and "(" in l)
    lines[head] = re.sub(r"\b" + re.escape(defined or cname(sym)) + r"\(", own_name(sym) + "(", lines[head], count=1)
    return "\n".join(lines) + "\n"


def register_text(names: list[str]) -> str:
    """The part's entry: every slot filled with its translation (the game calls it on loading the part)."""
    decls = "".join(f"extern OPENRAC_PART_SLOT void *{slot(n)};\nvoid {own_name(n)}(void);\n" for n in names)
    # On Windows the part is a DLL: the slots are the program's exports, and its entry its own.
    windows = ("#ifdef _WIN32\n#define OPENRAC_PART_SLOT __declspec(dllimport)\n"
               "#define OPENRAC_PART_ENTRY __declspec(dllexport)\n#else\n"
               "#define OPENRAC_PART_SLOT\n#define OPENRAC_PART_ENTRY\n#endif\n")
    sets = "".join(f"    {slot(n)} = (void *){own_name(n)};\n" for n in names)
    # On Android the game's runtime globals are its own (guest.h OPENRAC_PART_GLOBALS): the part
    # asks the game where they are.
    android = ("#if defined(__ANDROID__) && defined(OPENRAC_ASM_PART)\n#include \"openrac/guest.h\"\n"
               "void *openrac_asm_global(const char *name);\n"
               "#define OPENRAC_PART_DEFINE(type, name) type *openrac_part_##name;\n"
               "OPENRAC_PART_GLOBALS(OPENRAC_PART_DEFINE)\n#endif\n")
    find = ("#if defined(__ANDROID__) && defined(OPENRAC_ASM_PART)\n"
            "#define OPENRAC_PART_FIND(type, name) \\\n"
            "    if (!(openrac_part_##name = (type *)openrac_asm_global(\"openrac_\" #name))) return -1;\n"
            "    OPENRAC_PART_GLOBALS(OPENRAC_PART_FIND)\n#endif\n")
    return (f"{windows}{android}{decls}\nOPENRAC_PART_ENTRY int openrac_asm_register(void) {{\n{find}{sets}"
            f"    return {len(names)};\n}}\n")
