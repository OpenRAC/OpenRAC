"""The pvar words that hold a moby group's number, by class (rac1_group_fields.tsv): an imported
moby gets a group of its own for each, of the copies of that group's mobys (pack.py), as a
fleet turret's zone or a trooper's alert groups.

    groups.py write GEN LIBRARIES      the table again, from the port's generated C of a build
                                       (port/build/<preset>/games/rac1-ntsc/gen) and the version's
                                       libraries.tsv (its "The update of class N" hooks)

The game keeps no list of them, so they are read from its code: each place that takes a group
from the group table (its members walked as moby indices) is followed back, through locals and
through the parameters its callers pass, to a word of a moby's pvars. Each such word's place in
the pvars is worked out by the C compiler (clang), from the code's own types. A class gets the
words read anywhere in its update and the functions it calls. Standard library and clang.
"""

import re
import subprocess
import sys
import tempfile
from pathlib import Path

import spawns

TABLE = Path(__file__).with_name("rac1_group_fields.tsv")
TABLE_AT = re.compile(r"GREF\(gaddr, \(\(gaddr\)\((?:OPENRAC_LDATA\(\d+, )?\(?(0x[0-9A-F]{8})u\)?\)?\) \+ \(gaddr\)\(")
WALK = "& 32767"                       # a group's member: a moby index, its last with 0x8000 set
PVARS = re.compile(r"\(\((?:struct )?\w*\s?\*\)G\((?:[^()]|\([^()]*\)|\(\([^()]*\)\))*\)\)->pvars")
DEBUG = False


def table(path: Path = TABLE) -> dict[tuple[int, int], list[tuple[int, int]]]:
    """(class, level) -> its group words, (offset in its pvars, size in bytes)."""
    out: dict[tuple[int, int], list[tuple[int, int]]] = {}
    if not path.is_file():
        return out
    for line in path.read_text().splitlines():
        if not line or line.startswith("#"):
            continue
        class_id, level, words = (line.split("\t") + ["", ""])[:3]
        out[(int(class_id), int(level))] = [tuple(int(n) for n in w.split(":")) for w in words.split(",") if w]
    return out


_known: dict = {}


def fields(class_id: int, level: int, known: dict | None = None) -> list[tuple[int, int]]:
    """CLASS_ID's group words in LEVEL (its own update there, or the one every level shares)."""
    if known is None:
        if not _known:
            _known.update(table() or {(-1, -1): []})
        known = _known
    return known.get((class_id, level), known.get((class_id, -1), []))


def strip(text: str) -> str:
    """TEXT without the casts and brackets around it."""
    text = text.strip()
    while True:
        cast = re.fullmatch(r"\((?:\w+\s*\**\s*)+\)\s*\((.*)\)", text, re.S)
        if cast and balanced(cast.group(1)):
            text = cast.group(1).strip()
            continue
        if text.startswith("(") and text.endswith(")") and balanced(text[1:-1]):
            text = text[1:-1].strip()
            continue
        return text


def balanced(text: str) -> bool:
    depth = 0
    for c in text:
        depth += c == "("
        depth -= c == ")"
        if depth < 0:
            return False
    return depth == 0


def index_at(body: str, at: int) -> str:
    """The index of the group-table read whose '(gaddr)(' ends at AT: up to its '* (gaddr)sizeof'."""
    depth, i = 1, at
    while i < len(body) and depth:
        depth += body[i] == "("
        depth -= body[i] == ")"
        i += 1
    return body[at:i - 1]


def is_base(found: dict, callers: dict, function: str, name: str, memo: dict, depth: int = 0) -> bool:
    """NAME in FUNCTION holds a moby's pvars: set to them there, or passed them by its callers."""
    key = (function, name)
    if key in memo:
        return memo[key]
    memo[key] = False
    if depth > 8:
        return False
    f = found[function]

    def holds(where: str, text: str) -> bool:
        core = strip(text)
        if PVARS.fullmatch(core):
            return True
        word = core.strip("()")
        return bool(re.fullmatch(r"\w+", word)) and is_base(found, callers, where, word, memo, depth + 1)

    out = any(holds(function, rhs) for rhs in assigned(f.body, name))
    if not out and name in f.params:
        k = f.params.index(name)
        for caller in callers.get(function, []):
            body = found[caller].body
            for m in re.finditer(rf"\b{re.escape(function)}\s*\(", body):
                args = spawns.arguments(body, m.end() - 1)
                if k < len(args) and holds(caller, args[k]):
                    out = True
                    break
            if out:
                break
    memo[key] = out
    return out


def assigned(body: str, name: str) -> list[str]:
    """What NAME is set to in BODY."""
    out = []
    for m in re.finditer(rf"(?<![\w.>])\(?{re.escape(name)} = ", body):
        rest = body[m.end():]
        depth, i = 0, 0
        while i < len(rest):
            c = rest[i]
            if c in "([":
                depth += 1
            elif c in ")]":
                if depth == 0:
                    break
                depth -= 1
            elif c == ";" and depth == 0:
                break
            i += 1
        out.append(rest[:i])
    return out


def write(gen: Path, libraries: Path, path: Path = TABLE) -> int:
    found = spawns.functions(gen)
    # Pvar reads, as (function, expression with its pvars base named PVARS_BASE, its C file).
    reads: dict[str, set[str]] = {}
    sinks: list[tuple[str, str]] = []
    # The group tables (one a level): a table whose entry, read into a local, is walked as a
    # group's members there; then every read of those tables is a group's number.
    tables = set()
    for f in found.values():
        if WALK not in f.body:
            continue
        walked = {w for line in f.body.split("\n") if WALK in line for w in re.findall(r"\w+", line)}
        for m in TABLE_AT.finditer(f.body):
            into = re.search(r"(\w+) = \(gaddr\)\(\(?$", f.body[max(0, m.start() - 80):m.start()])
            if into and into.group(1) in walked:
                tables.add(m.group(1))
    for name, f in found.items():
        for m in TABLE_AT.finditer(f.body):
            if m.group(1) in tables:
                sinks.append((name, index_at(f.body, m.end())))
    callers: dict[str, list[str]] = {}
    for name, f in found.items():
        for callee in f.calls:
            callers.setdefault(callee, []).append(name)
    group_params: set[tuple[str, int]] = set()
    memo: dict = {}
    todo = [(name, text, 0) for name, text in sinks]
    seen = set()
    while todo:
        name, text, depth = todo.pop()
        if (name, text) in seen or depth > 12:
            continue
        seen.add((name, text))
        f = found[name]
        core = strip(text)
        word = core.strip("()")
        if re.fullmatch(r"\w+", word):
            if word in f.params:
                k = f.params.index(word)
                if (name, k) not in group_params:
                    group_params.add((name, k))
                    for caller in callers.get(name, []):
                        body = found[caller].body
                        for m in re.finditer(rf"\b{re.escape(name)}\s*\(", body):
                            args = spawns.arguments(body, m.end() - 1)
                            if k < len(args):
                                todo.append((caller, args[k], depth + 1))
                continue
            for rhs in assigned(f.body, word):
                todo.append((name, rhs, depth + 1))
            continue
        # A read of a moby's pvars: through a local set to them, or straight from the moby.
        base = PVARS.search(core)
        names = [n for n in set(re.findall(r"\b[a-z_]\w*\b", core))
                 if not n.startswith(("game_", "openrac_")) and n not in ("gaddr", "sizeof", "offsetof")
                 and is_base(found, callers, name, n, memo)]
        if base or names:
            expr = PVARS.sub("PVARS_BASE", core)
            for n in names:
                expr = re.sub(rf"\b{re.escape(n)}\b", "PVARS_BASE", expr)
            reads.setdefault(name, set()).add(expr)
        elif DEBUG:
            print(f"untraced {name}: {core[:160]}", file=sys.stderr)
    offsets = measure(gen, found, reads)
    rows: dict[tuple[int, int], set[tuple[int, int]]] = {}
    for function, level, class_id in spawns.hooks(libraries):
        words = set()
        for f in spawns.closure(found, function):
            words |= offsets.get(f, set())
        if words:
            rows.setdefault((class_id, level), set()).update(words)
    lines = ["# class\tlevel (-1: every level's)\tits pvar words that hold a moby group, offset:size "
             "(editor/groups.py, from the game's code)"]
    lines += [f"{c}\t{lv}\t{','.join(f'{o}:{s}' for o, s in sorted(w))}" for (c, lv), w in sorted(rows.items())]
    path.write_text("\n".join(lines) + "\n")
    return len(rows)


def measure(gen: Path, found: dict, reads: dict[str, set[str]]) -> dict[str, set[tuple[int, int]]]:
    """Each read's (offset, size), by clang: the read, its pvars base made 0, compiled in its file."""
    by_file: dict[Path, list[tuple[str, str, str, str]]] = {}
    for name, exprs in reads.items():
        for expr in exprs:
            if "PVARS_BASE" not in expr:
                continue
            address, size = place(expr)
            if address is None and DEBUG:
                print(f"unplaced {name}: {expr[:160]}", file=sys.stderr)
            if address is None:
                continue
            # An array's element by a number worked out while it runs (on_lists[i]): every element.
            count = "1"
            each = re.search(r"\(gaddr\)offsetof\(((?:struct )?\w+), (\w+)\)\)\) \+ \(gaddr\)\(([a-z_]\w*)\) \* "
                             r"\(gaddr\)sizeof\(([\w\s\*]+?)\)", address)
            if each:
                t, member, variable, element = each.groups()
                address = address.replace(f"(gaddr)({variable})", "(gaddr)(0)")
                count = f"sizeof((({t} *)0)->{member}) / sizeof({element})"
            by_file.setdefault(found[name].file, []).append((name, address, size, count))
    out: dict[str, set[tuple[int, int]]] = {}
    port = next((p for p in gen.parents if (p / "runtime" / "include").is_dir()), gen)
    for file, items in by_file.items():
        text = file.read_text(errors="replace")
        probe = "\n#define PVARS_BASE 0\n#include <stddef.h>\nconst unsigned long openrac_probe[] = {\n"
        probe += ",\n".join(f"(unsigned long)({a}), (unsigned long)({s}), (unsigned long)({n})"
                             for _, a, s, n in items) + "};\n"
        with tempfile.TemporaryDirectory() as scratch:
            source = Path(scratch) / "probe.c"
            source.write_text(text + probe)
            done = subprocess.run(
                ["clang", "-S", "-emit-llvm", "-O0", "-w", "-o", "-", "-I", str(port / "runtime" / "include"),
                 "-I", str(port / "game" / "common" / "include"), "-I", str(gen), "-I", str(file.parent),
                 str(source)], capture_output=True, text=True)
        found_values = re.search(r"@openrac_probe = \S+ constant \[\d+ x i64\] \[(.*)\]", done.stdout)
        if not found_values:
            print(f"{file}: clang could not measure ({done.stderr.strip()[:200]})", file=sys.stderr)
            continue
        values = [int(v) for v in re.findall(r"i64 (-?\d+)", found_values.group(1))]
        for (name, _, _, _), (offset, size, count) in zip(items, zip(values[::3], values[1::3], values[2::3])):
            if 0 <= offset < 0x1000 and size in (1, 2, 4) and 0 < count <= 32:
                out.setdefault(name, set()).update((offset + k * size, size) for k in range(count))
    return out


def place(expr: str) -> tuple[str | None, str]:
    """The address and size of the pvar word EXPR reads: GREF(T, ADDR) or ((Type *)G(base))->field."""
    m = re.fullmatch(r"GREF\(([\w\s\*]+?), (.*)\)", expr, re.S)
    if m and balanced(m.group(2)):
        return m.group(2), f"sizeof({m.group(1)})"
    m = re.fullmatch(r"\(\(((?:struct )?\w+) \*\)G\(\(?PVARS_BASE\)?\)\)->(\w+)", expr)
    if m:
        return f"offsetof({m.group(1)}, {m.group(2)})", f"sizeof((({m.group(1)} *)0)->{m.group(2)})"
    m = re.fullmatch(r"\(\(((?:struct )?\w+) \*\)G\(\(?PVARS_BASE\)?\)\)->(\w+)\[(\d+)\]", expr)
    if m:
        t, f, i = m.groups()
        return f"offsetof({t}, {f}) + {i} * sizeof((({t} *)0)->{f}[0])", f"sizeof((({t} *)0)->{f}[0])"
    return None, ""


if __name__ == "__main__":
    DEBUG = "--debug" in sys.argv
    sys.argv = [a for a in sys.argv if a != "--debug"]
    if len(sys.argv) != 4 or sys.argv[1] != "write":
        sys.exit(__doc__)
    print(f"{write(Path(sys.argv[2]), Path(sys.argv[3]))} rows")
