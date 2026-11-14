"""The moby classes a class's update can make (rac1_spawns.tsv): an imported class brings those its
new level lacks (pack.py, foreign_classes.py), as a turret brings its shots.

    spawns.py write GEN LIBRARIES      the table again, from the port's generated C of a build
                                       (port/build/<preset>/games/rac1-ntsc/gen) and the version's
                                       libraries.tsv (its "The update of class N" hooks)

The table is read from the game's code, so it is the same for every disc: each class's update
and every function it calls, and theirs, and each CreateMoby call there with a class number
written in the code (and each class whose sound it plays, which must be in the level too). A class an update takes from its data (a spawner's pvars) is not found.
Standard library only.
"""

import re
import sys
from pathlib import Path

TABLE = Path(__file__).with_name("rac1_spawns.tsv")
CREATE = "game_FUN_0020c4f8"     # CreateMoby (rac1-ntsc's name)
# Plays a sound of a class's (its 4th argument): the class's sounds are found through its slot, so
# a class whose sounds an update plays must be in the level as one it makes.
SOUND = "game_FUN_L00_0028dc90"


def table(path: Path = TABLE) -> dict[tuple[int, int], set[int]]:
    """(class, level) -> the classes its update can make."""
    out: dict[tuple[int, int], set[int]] = {}
    if not path.is_file():
        return out
    for line in path.read_text().splitlines():
        if not line or line.startswith("#"):
            continue
        class_id, level, made = (line.split("\t") + ["", ""])[:3]
        out[(int(class_id), int(level))] = {int(c) for c in made.split(",") if c}
    return out


def made_by(class_id: int, level: int, known: dict[tuple[int, int], set[int]] | None = None) -> set[int]:
    """The classes CLASS_ID's update in LEVEL can make, and theirs (in LEVEL), CLASS_ID left out."""
    known = table() if known is None else known
    out, todo = set(), [class_id]
    while todo:
        one = todo.pop()
        for made in known.get((one, level), known.get((one, -1), set())):
            if made not in out and made != class_id:
                out.add(made)
                todo.append(made)
    return out


def arguments(text: str, start: int) -> list[str]:
    """The arguments of the call whose '(' is at START in TEXT, as written."""
    out, depth, at = [], 0, start + 1
    for i in range(start, len(text)):
        c = text[i]
        if c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
            if depth == 0:
                out.append(text[at:i].strip())
                return out
        elif c == "," and depth == 1:
            out.append(text[at:i].strip())
            at = i + 1
    return out


class Function:
    """A function of the generated C: its file, parameter names and body."""
    def __init__(self, file: Path, params: list[str], body: str):
        self.file, self.params, self.body = file, params, body
        self.calls = set(re.findall(r"\b(game_\w+)\s*\(", body))


def functions(gen: Path) -> dict[str, Function]:
    """Every function of GEN's C (not prep/, the decomp's input), by name."""
    head = re.compile(r"^(?:static\s+)?[A-Za-z_][\w\s\*]*?\b(game_\w+)\s*\(([^;{]*)\)\s*$")
    out: dict[str, Function] = {}
    for file in sorted(gen.rglob("*.c")):
        if "prep" in file.relative_to(gen).parts:
            continue
        lines = file.read_text(errors="replace").split("\n")
        i = 0
        while i < len(lines):
            found = head.match(lines[i])
            if found and i + 1 < len(lines) and lines[i + 1].startswith("{"):
                body, depth, j = [], 0, i + 1
                while j < len(lines):
                    depth += lines[j].count("{") - lines[j].count("}")
                    body.append(lines[j])
                    j += 1
                    if depth == 0:
                        break
                name = found.group(1)
                params = [p.split()[-1].lstrip("*") if p.strip() not in ("", "void") else ""
                          for p in found.group(2).split(",")]
                out[name] = Function(file, params, "\n".join(body))
                out[name].calls.discard(name)
                i = j
                continue
            i += 1
    return out


def hooks(libraries: Path) -> list[tuple[str, int, int]]:
    """(function, level or -1 for every level's, class) of each "The update of class N" hook."""
    hook = re.compile(r"^(FUN_(?:L(\d\d)_)?[0-9a-f]{8})\t\S+\t\S+\thook\tThe update of class (?:[a-z_0-9]+ \()?(\d+)")
    out = []
    for line in libraries.read_text().splitlines():
        found = hook.match(line)
        if found:
            level = found.group(2)
            out.append((found.group(1), int(level) if level not in (None, "00") else -1, int(found.group(3))))
    return out


def closure(found: dict[str, Function], function: str) -> set[str]:
    """FUNCTION's host C (its __game body when wrapped) and every function it calls, and theirs."""
    start = next((n for n in (f"game_{function}__game", f"game_{function}") if n in found), None)
    if start is None:
        return set()
    seen, todo = {start}, [start]
    while todo:
        for callee in found[todo.pop()].calls:
            if callee in found and callee not in seen:
                seen.add(callee)
                todo.append(callee)
    return seen


def write(gen: Path, libraries: Path, path: Path = TABLE) -> int:
    """The table from GEN and LIBRARIES; returns its rows."""
    literal = re.compile(r"^\(int\)\((\d+)\)$")
    found = functions(gen)
    params = {name: f.params for name, f in found.items()}
    bodies = {name: f.body for name, f in found.items()}
    makes: dict[str, set[int]] = {name: set() for name in found}
    # The class each call makes: a number written in the call (CreateMoby((int)(29))), or one passed
    # to a function that hands that argument on to CreateMoby (a gunship's part maker, any depth).
    makers: dict[str, int] = {CREATE: 0, SOUND: 3}
    while True:
        more = False
        for name, text in bodies.items():
            for m in re.finditer(r"\b(game_\w+)\s*\(", text):
                callee = m.group(1)
                if callee not in makers:
                    continue
                args = arguments(text, m.end() - 1)
                if makers[callee] >= len(args):
                    continue
                arg = args[makers[callee]]
                number = literal.match(arg)
                if number:
                    makes[name].add(int(number.group(1)))
                elif name not in makers:
                    passed = re.fullmatch(r"\(int\)\((\w+)\)|(\w+)", arg)
                    word = passed and (passed.group(1) or passed.group(2))
                    if word in params.get(name, []):
                        makers[name] = params[name].index(word)
                        more = True
        if not more:
            break
    rows: dict[tuple[int, int], set[int]] = {}
    for function, level, class_id in hooks(libraries):
        made = set().union(*(makes.get(f, set()) for f in closure(found, function))) - {class_id}
        if made:   # level -1: the update every level shares (level 0's code)
            rows.setdefault((class_id, level), set()).update(made)
    lines = ["# class\tlevel (-1: every level's)\tthe classes its update can make (editor/spawns.py, from the game's code)"]
    lines += [f"{c}\t{lv}\t{','.join(str(m) for m in sorted(made))}" for (c, lv), made in sorted(rows.items())]
    path.write_text("\n".join(lines) + "\n")
    return len(rows)


if __name__ == "__main__":
    if len(sys.argv) != 4 or sys.argv[1] != "write":
        sys.exit(__doc__)
    print(f"{write(Path(sys.argv[2]), Path(sys.argv[3]))} rows")
