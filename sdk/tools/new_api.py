#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 the OpenRAC contributors
"""Add a new call to the end of openrac_api, with every place it needs, in one go.

    python3 sdk/tools/new_api.py world moby_glow --returns int \\
        --params "openrac_mod* mod, uint32_t moby, float strength" \\
        --about "Makes a moby glow ... Returns OPENRAC_OK, or OPENRAC_E_UNKNOWN for a moby that is gone."

It writes the parts that always look the same (docs/port/MOD_API.md says why each is there):

    sdk/include/openrac/openrac_mod.h          the member, last in openrac_api, "Since <major>.<open>."
    sdk/api/members.tsv                        its row: area, <major>.<open>, experimental (or --stable)
    port/game/common/mod_api/internal.h        the declaration of api_<name>
    port/game/common/mod_api/<area>.cpp        api_<name>, a stub that does nothing yet
    port/game/common/mods.cpp                  g_api.<name> = api_<name>;

The new member joins the open minor, the one after the last SDK release (bump_api.py), so calls
that two pull requests add meanwhile get the same "since". Then it prints what is left by hand:
the stub's body, the C++ wrapper in mod.hpp, a test, and bump_api.py minor --note (the note for
the changelog; it sets the version in every place).
Stdlib only.
"""
from __future__ import annotations

import argparse
import re
import sys
import textwrap
from pathlib import Path

import bump_api
import gen_api_lock
import gen_reference

ROOT = Path(__file__).resolve().parents[2]
HEADER = Path("sdk/include/openrac/openrac_mod.h")
MEMBERS = Path("sdk/api/members.tsv")
INTERNAL = Path("port/game/common/mod_api/internal.h")
AREA_FILE = "port/game/common/mod_api/{area}.cpp"
MODS = Path("port/game/common/mods.cpp")
NAMESPACE_END = "}  // namespace openrac::mods::detail"
FILL_END = "    g_api_without_native_vulkan = g_api;"
WIDTH = 100


def read(root: Path, path: Path) -> str:
    return (root / path).read_text()


def header_member(name: str, returns: str, params: str, about: str, since: str) -> str:
    """The member as the header writes them: a comment, then the pointer, on one line when it fits."""
    comment = textwrap.wrap(f"{about.strip().rstrip('.')}. Since {since}.", WIDTH - 7,
                            break_long_words=False, break_on_hyphens=False)
    lines = ["    /* " + comment[0]] + ["     * " + line for line in comment[1:]]
    lines[-1] += " */"
    one = f"    {returns} (*{name})({params});"
    if len(one) <= WIDTH:
        return "\n".join(lines + [one])
    args = [a.strip() for a in params.split(",")]
    return "\n".join(lines + [f"    {returns} (*{name})(", *(f"        {a}," for a in args[:-1]),
                              f"        {args[-1]}", "    );"])


def stub_body(returns: str) -> str:
    if returns == "void":
        return ""
    if returns.endswith("*"):
        return "    return nullptr;\n"
    if returns == "int":
        return "    return OPENRAC_E_UNKNOWN;\n"
    return "    return {};\n"


def tidy(c: str) -> str:
    """A type or a parameter in the header's style: one space between words, `type* name`."""
    c = re.sub(r"\s*\*\s*", "*", " ".join(c.split()))
    return re.sub(r"\*(?=\w)", "* ", c)


def param_names(params: list[str]) -> list[str]:
    return [re.sub(r"\[.*\]$", "", re.split(r"[\s*]+", p)[-1]) for p in params]


def cpp_type(text: str) -> str:
    """uint32_t and the like as the port's C++ writes them."""
    return re.sub(r"\b(u?int(?:8|16|32|64)_t)\b", r"std::\1", text)


def cpp_signature(head: str, params: list[str], tail: str) -> str:
    """`head(params)tail` as port/.clang-format lays it out: one line when it fits, else the
    parameters on the next line, else one parameter a line."""
    one = f"{head}({', '.join(params)}){tail}"
    if len(one) <= WIDTH:
        return one
    joined = "    " + ", ".join(params)
    if len(joined) <= WIDTH:
        return f"{head}(\n{joined}\n){tail}"
    return f"{head}(\n" + ",\n".join("    " + p for p in params) + f"\n){tail}"


def insert_before_last(text: str, anchor: str, add: str, where: Path, blank: bool = False) -> str:
    """`add` put just before the last `anchor`; with `blank`, one empty line before it and after it."""
    at = text.rfind(anchor)
    if at < 0:
        raise SystemExit(f"{where}: no {anchor!r}")
    if blank:
        return text[:at].rstrip("\n") + "\n\n" + add.strip("\n") + "\n\n" + text[at:]
    return text[:at] + add + text[at:]


# Names a member cannot have: the header is C and C++ both.
KEYWORDS = {"alignas", "alignof", "and", "asm", "auto", "bool", "break", "case", "catch", "char", "class",
            "const", "constexpr", "continue", "default", "delete", "do", "double", "else", "enum", "explicit",
            "export", "extern", "false", "float", "for", "friend", "goto", "if", "inline", "int", "long",
            "mutable", "namespace", "new", "noexcept", "not", "nullptr", "operator", "or", "private",
            "protected", "public", "register", "restrict", "return", "short", "signed", "sizeof", "static",
            "struct", "switch", "template", "this", "throw", "true", "try", "typedef", "typeid", "typename",
            "union", "unsigned", "using", "virtual", "void", "volatile", "while", "xor"}


def add_call(root: Path, area: str, name: str, returns: str, params: str, about: str,
             experimental: bool) -> str:
    if area not in gen_api_lock.AREAS:
        raise SystemExit(f"no area {area!r}; the areas are {', '.join(gen_api_lock.AREAS)}")
    if not re.fullmatch(r"[a-z][a-z0-9]*(?:_[a-z0-9]+)*", name) or len(name) > 40 or name in KEYWORDS:
        raise SystemExit(f"{name!r}: a member's name is lower case words joined by _, at most 40 characters, "
                         "and not a C or C++ keyword")
    returns = tidy(returns)
    if not re.fullmatch(r"[A-Za-z_][\w ]*\**", returns):
        raise SystemExit(f"--returns {returns!r}: a plain C type (a function pointer needs a typedef)")
    if "(" in params or ")" in params:
        raise SystemExit("--params: a function pointer needs a typedef (openrac_<thing>_fn), as the header's others")
    args = [tidy(p) for p in params.split(",")]
    if args[0] != "openrac_mod* mod" or any(not re.fullmatch(r"[A-Za-z_][\w *]*\b\w+(\[\w*\])?", a) for a in args):
        raise SystemExit("--params: C parameters with names, the first 'openrac_mod* mod' (every call says which "
                         "mod asks)")
    names = param_names(args)
    if len(set(names)) != len(names):
        raise SystemExit("--params: two parameters have one name")
    about = " ".join(about.split())
    if not about or "*/" in about or "/*" in about:
        raise SystemExit("--about: what the call does and returns, without /* or */")

    header = read(root, HEADER)
    members = read(root, MEMBERS)
    internal = read(root, INTERNAL)
    mods = read(root, MODS)
    area_path = Path(AREA_FILE.format(area=area))
    if not (root / area_path).exists():
        raise SystemExit(f"{area_path}: no such file")
    area_text = read(root, area_path)
    if any(line.split("\t")[0] == name for line in members.splitlines()):
        raise SystemExit(f"{MEMBERS}: {name} is already a member")
    if re.search(rf"\bapi_{name}\b", internal + mods):
        raise SystemExit(f"api_{name} is already in the port's code")
    major_at = re.search(r"^#define OPENRAC_MOD_API_MAJOR (\d+)$", header, re.M)
    if not major_at or "} openrac_api;" not in header:
        raise SystemExit(f"{HEADER}: no OPENRAC_MOD_API_MAJOR or no openrac_api")
    major = int(major_at.group(1))
    since = f"{major}.{open_minor(root, major)}"

    # Every file is worked out before any is written, so a refusal leaves the tree as it was.
    end = header.index("} openrac_api;")
    out = {HEADER: header[:end] + "\n" + header_member(name, returns, ", ".join(args), about, since) + "\n"
           + header[end:]}
    status = "experimental" if experimental else "stable"
    out[MEMBERS] = members.rstrip("\n") + f"\n{name}\t{area}\t{since}\t{status}\n"
    cpp_args = [cpp_type(a) for a in args]
    head = f"{cpp_type(returns)} api_{name}"
    out[INTERNAL] = insert_before_last(internal, NAMESPACE_END, cpp_signature(head, cpp_args, ";"), INTERNAL,
                                       blank=True)
    stub = (f"// openrac_api.{name}. Since {since}.\n{cpp_signature(head, cpp_args, ' {')}\n"
            f"    // {gen_reference.STUB_MARK}; until then it does nothing.\n"
            + "".join(f"    static_cast<void>({n});\n" for n in names)
            + f"{stub_body(returns)}}}\n\n")
    out[area_path] = insert_before_last(area_text, NAMESPACE_END, stub, area_path, blank=True)
    out[MODS] = insert_before_last(mods, FILL_END, f"    g_api.{name} = api_{name};\n", MODS)
    for path, text in out.items():
        (root / path).write_text(text)
    return since


def open_minor(root: Path, major: int) -> int:
    """The minor a new member joins: the one after the last stable release (bump_api.py), 0 before
    the major's first."""
    try:
        release = bump_api.last_release(root, major)
    except SystemExit as e:  # the tags could not be read (no network for a shallow clone)
        minor = bump_api.header_api(root)[1] + 1
        print(f"warning: {e}; guessed {major}.{minor} (bump_api.py --check tells)", file=sys.stderr)
        return minor
    return release.version[1] + 1 if release else 0


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("area", help="the member's area: " + ", ".join(gen_api_lock.AREAS))
    ap.add_argument("name", help="the member's name, lower case words joined by _")
    ap.add_argument("--returns", default="int", help="its C return type (default int: OPENRAC_OK or an error)")
    ap.add_argument("--params", required=True, help='its C parameters, first "openrac_mod* mod"')
    ap.add_argument("--about", required=True, help="what it does and returns, for the header's comment")
    ap.add_argument("--stable", action="store_true",
                    help="stable from the start (a new call is experimental until a release has used it)")
    ap.add_argument("--root", type=Path, default=ROOT, help=argparse.SUPPRESS)
    args = ap.parse_args(argv)
    since = add_call(args.root, args.area, args.name, args.returns, args.params, args.about, not args.stable)
    print(textwrap.dedent(f"""\
        openrac_api.{args.name} added (since {since}, area {args.area}). Left by hand:
          1. port/game/common/mod_api/{args.area}.cpp: the body of api_{args.name}
          2. sdk/include/openrac/mod.hpp: its C++ wrapper (an older port's table is shorter: a has_<name>() that checks m_api->size)
          3. a test in the area's port/tests/mod_*_test.cpp, else mod_loader_test.cpp (docs/port/MOD_API.md)
          4. python3 sdk/tools/bump_api.py minor --note "what it does ({args.name})" (the changelog's note; sets the version)
        """))
    return 0


if __name__ == "__main__":
    sys.exit(main())
