"""The pvar words that hold a place in the world, by class (rac1_position_fields.tsv): an imported
moby's are moved with it (foreign_classes.py), as its paths are, so a lift still stops at its own
floors and not at the heights of its old level.

    positions.py write GEN LIBRARIES   the table again, from the port's generated C of a build
                                       (port/build/<preset>/games/rac1-ntsc/gen) and the version's
                                       libraries.tsv (its "The update of class N" hooks)

A word is a place when the game sets a moby's position to it: `pos.z = vars->raised_z`, followed
back through locals and the arguments callers pass as groups.py does. A word the code only adds to
a position (a speed) is not one. A class gets the words of its update and the functions it calls.
Standard library and clang.
"""

import re
import sys
from pathlib import Path

import groups
import spawns

TABLE = Path(__file__).with_name("rac1_position_fields.tsv")
SET = re.compile(r"offsetof\(struct Moby, pos\)\)+->([xyz])\) = ")


def table(path: Path = TABLE) -> dict[tuple[int, int], list[tuple[int, str]]]:
    """(class, level) -> its place words, (offset in its pvars, axis x, y or z)."""
    out: dict[tuple[int, int], list[tuple[int, str]]] = {}
    if not path.is_file():
        return out
    for line in path.read_text().splitlines():
        if not line or line.startswith("#"):
            continue
        class_id, level, words = (line.split("\t") + ["", ""])[:3]
        out[(int(class_id), int(level))] = [(int(w.split(":")[0]), w.split(":")[1]) for w in words.split(",") if w]
    return out


_known: dict = {}


def fields(class_id: int, level: int, known: dict | None = None) -> list[tuple[int, str]]:
    """CLASS_ID's place words in LEVEL (its own update there, or the one every level shares)."""
    if known is None:
        if not _known:
            _known.update(table() or {(-1, -1): []})
        known = _known
    return known.get((class_id, level), known.get((class_id, -1), []))


def right_side(body: str, at: int) -> str:
    """The expression from AT to the end of its statement."""
    depth, i = 0, at
    while i < len(body):
        c = body[i]
        if c == "(":
            depth += 1
        elif c == ")":
            if depth == 0:
                break
            depth -= 1
        elif c == ";" and depth == 0:
            break
        i += 1
    return body[at:i]


def terms(expr: str) -> list[tuple[str, str]]:
    """EXPR's top-level terms with their signs: "a - (b + c) + d" -> + a, - (b + c), + d."""
    out, depth, start, sign = [], 0, 0, "+"
    for i, c in enumerate(expr):
        depth += c == "("
        depth -= c == ")"
        if depth == 0 and c in "+-" and i > 0 and expr[i - 1] == " " and expr[i + 1:i + 2] == " ":
            out.append((sign, expr[start:i].strip()))
            sign, start = c, i + 1
    out.append((sign, expr[start:].strip()))
    return out


def scaled(expr: str) -> bool:
    """EXPR multiplies or divides at its top level (an offset scaled by something)."""
    depth = 0
    for i, c in enumerate(expr):
        depth += c == "("
        depth -= c == ")"
        if depth == 0 and c in "*/" and expr[i - 1:i] == " ":
            return True
    return False


def differences(expr: str) -> list[tuple[str, str]]:
    """Each "(a) - (b)" in EXPR, as (a, b)."""
    out = []
    for m in re.finditer(r"\) - \(", expr):
        depth, i = 0, m.start()
        while i >= 0:
            depth += expr[i] == ")"
            depth -= expr[i] == "("
            if depth == 0:
                break
            i -= 1
        depth, j = 0, m.end() - 1
        while j < len(expr):
            depth += expr[j] == "("
            depth -= expr[j] == ")"
            if depth == 0:
                break
            j += 1
        out.append((expr[i:m.start() + 1], expr[m.end() - 1:j + 1]))
    return out


def place_of(expr: str) -> list[str]:
    """The pvar reads EXPR sets a position from: the read itself, or the one read it adds a number
    or a product to (`base_z - 0.7f`, `bottom + (top - bottom) * t`, and then top). None when EXPR
    also reads the position (a move by a speed) or adds another word that may be the place."""
    expr = groups.strip(expr)
    if "offsetof(struct Moby, pos)" in expr:
        return []
    if groups.place(expr)[0] is not None:
        return [expr]
    reads, others = [], []
    for sign, term in terms(expr):
        core = groups.strip(term)
        if groups.place(core)[0] is not None and "PVARS_BASE" in core:
            (reads if sign == "+" else others).append(core)
        elif not (re.fullmatch(r"-?[\d.]+f?", core) or scaled(core) or re.match(r"\(?(?:game_|func_)\w+\(", core)):
            others.append(core)    # a plain word: maybe the place, the read only an offset
    if len(reads) != 1 or others:
        return []
    # bottom + (top - bottom) * t: top is a place as well.
    for sign, term in terms(expr):
        for a, b in differences(term):
            if groups.strip(b) == reads[0] and "PVARS_BASE" in a and groups.place(groups.strip(a))[0]:
                reads.append(groups.strip(a))
    return reads


def write(gen: Path, libraries: Path, path: Path = TABLE) -> int:
    found = spawns.functions(gen)
    offsets: dict[str, set[tuple[int, str]]] = {}
    for axis in "xyz":
        sinks = [(name, right_side(f.body, m.end())) for name, f in found.items()
                 for m in SET.finditer(f.body) if m.group(1) == axis]
        reads = {name: {p for e in exprs for p in place_of(e)} for name, exprs in groups.trace(found, sinks).items()}
        for function, words in groups.measure(gen, found, reads).items():
            offsets.setdefault(function, set()).update((o, axis) for o, size in words if size == 4)
    rows: dict[tuple[int, int], set[tuple[int, str]]] = {}
    for function, level, class_id in spawns.hooks(libraries):
        words = set()
        for f in spawns.closure(found, function):
            words |= offsets.get(f, set())
        if words:
            rows.setdefault((class_id, level), set()).update(words)
    lines = ["# class\tlevel (-1: every level's)\tits pvar words that hold a place in the world, "
             "offset:axis (editor/positions.py, from the game's code)"]
    lines += [f"{c}\t{lv}\t{','.join(f'{o}:{a}' for o, a in sorted(w))}" for (c, lv), w in sorted(rows.items())]
    path.write_text("\n".join(lines) + "\n")
    return len(rows)


if __name__ == "__main__":
    groups.DEBUG = "--debug" in sys.argv
    sys.argv = [a for a in sys.argv if a != "--debug"]
    if len(sys.argv) != 4 or sys.argv[1] != "write":
        sys.exit(__doc__)
    print(f"{write(Path(sys.argv[2]), Path(sys.argv[3]))} rows")
