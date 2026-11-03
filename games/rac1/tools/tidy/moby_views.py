#!/usr/bin/env python3
"""
Find byte pointers that are certainly a Moby, and give them Moby fields.

  python3 tools/tidy/moby_views.py [--write] [--list] [FILE...]

For each function, a local or parameter named like a moby (`m`, `moby`, `mob`, `self`) that is a byte or void pointer
is taken as a Moby only if its raw accesses say so. At most a quarter of them may disagree with the layout, and one of
these must hold:
  - an offset is a field only a moby has (pclass, update, pvars, oclass, save_id, command);
  - three or more different fields are used, and not only the position (any vector could look like that);
  - the function's name says moby, and two or more fields are used.
A variable set from a particle call is never a Moby.
Any other variable name needs both: a field only a moby has, and three or more different fields.
--skip=A,B leaves functions raw, by address or readable name. The conversion itself is tools/tidy/rawstruct.py. The gate must still pass: run tools/gate_loop.py after --write.
"""
import collections
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import prune_externs as pe  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
NAMES = ("m", "moby", "mob", "self")
# offset: (size, 'f' float, 'p' pointer or 'i' integer)
FIELDS = {0x00: (4, "f"), 0x04: (4, "f"), 0x08: (4, "f"), 0x0C: (4, "f"), 0x10: (4, "f"), 0x14: (4, "f"), 0x18: (4, "f"),
          0x1C: (4, "f"), 0x20: (1, "i"), 0x21: (1, "i"), 0x22: (1, "i"), 0x23: (1, "i"), 0x24: (4, "p"), 0x28: (4, "p"),
          0x2C: (4, "f"), 0x30: (1, "i"), 0x31: (1, "i"), 0x32: (2, "i"), 0x34: (2, "i"), 0x36: (2, "i"), 0x38: (8, "i"),
          0x40: (4, "f"), 0x44: (4, "f"), 0x48: (4, "f"), 0x4C: (4, "f"), 0x50: (1, "i"), 0x51: (1, "i"), 0x52: (1, "i"),
          0x53: (1, "i"), 0x54: (4, "f"), 0x58: (4, "f"), 0x64: (4, "p"), 0x68: (4, "p"), 0x6C: (4, "p"), 0x70: (1, "i"),
          0x71: (1, "i"), 0x72: (1, "i"), 0x73: (1, "i"), 0x74: (4, "p"), 0x78: (4, "p"), 0x7C: (1, "i"), 0x7D: (1, "i"),
          0x7E: (1, "i"), 0x7F: (1, "i"), 0x80: (4, "i"), 0x84: (4, "f"), 0x86: (2, "i"), 0x88: (4, "f"), 0x90: (4, "i"),
          0x94: (4, "i"), 0x98: (4, "i"), 0xA4: (1, "i"), 0xA6: (2, "i"), 0xB0: (1, "i"), 0xB1: (1, "i"), 0xB2: (2, "i"),
          0xB4: (2, "i"), 0xB6: (2, "i"), 0xB8: (4, "p"), 0xBC: (1, "i"), 0xBD: (1, "i")}
for base in (0xC0, 0xD0, 0xE0):
    FIELDS.update({base + i: (4, "f") for i in (0, 4, 8, 12)})
ONLY_A_MOBY = {0x24, 0x74, 0x78, 0xA6, 0xB2, 0xBC}
POSITION = {0x10, 0x14, 0x18, 0x1C}
SIZES = {"char": 1, "u8": 1, "s8": 1, "short": 2, "s16": 2, "u16": 2, "int": 4, "s32": 4, "u32": 4, "float": 4, "f32": 4,
         "long": 8, "s64": 8, "u64": 8}
COMMENT = re.compile(r"/\*.*?\*/|//[^\n]*", re.S)
FUNC = re.compile(r"^[A-Za-z_][\w \*]*?\b(\w+)\s*\(([^;{}]*)\)\s*\{", re.M)
RAW = (r"\*\(\s*(?:volatile\s+)?(?:unsigned\s+|signed\s+)?(\w+)\s*(\*+)\s*\)\s*\(?\s*(?:\(\s*(?:unsigned\s+)?char\s*\*\s*\)\s*)?"
       r"\b{v}\s*\+\s*(0x[0-9A-Fa-f]+|\d+)\)?")
BYTE_POINTER = r"(?:const\s+)?(?:unsigned\s+char|char|u8|s8|void)\s*\*\s*"


def access_fits(typ, stars, offset):
    field = FIELDS.get(offset)
    if not field:
        return False
    size, kind = field
    if len(stars) > 1:
        return kind == "p"
    if typ in ("float", "f32"):
        return kind == "f"
    return SIZES.get(typ) == size and kind != "f"


def is_moby(body, var, function=""):
    """(accesses, fit) if the raw accesses of var say it is a Moby, else None."""
    pairs = [(m.group(1), m.group(2), int(m.group(3), 0)) for m in re.finditer(RAW.format(v=var), body)]
    if not pairs or re.search(rf"\b{var}\s*=\s*(?:\([^()]*\)\s*)?\w*part\w*\s*\(", body, re.I):
        return None
    fits = [p for p in pairs if access_fits(*p)]
    offsets = {p[2] for p in fits}
    if len(pairs) - len(fits) > len(pairs) / 4:
        return None
    if var not in NAMES:
        return (len(pairs), len(fits)) if offsets & ONLY_A_MOBY and len(offsets) >= 3 else None
    named = "moby" in function.lower() and len(offsets) >= 2
    if offsets & ONLY_A_MOBY or named or (len(offsets) >= 3 and offsets - POSITION):
        return len(pairs), len(fits)
    return None


def declared_as_bytes(params, body, var):
    pattern = rf"{BYTE_POINTER}{var}\b"
    in_params = re.search(pattern, params)
    locals_found = re.findall(rf"^\s*{pattern}", body, re.M)
    return bool(in_params) + len(locals_found) == 1


def candidates(text):
    """{var: [function]}"""
    found = collections.defaultdict(list)
    clean = COMMENT.sub(" ", text)
    starts = list(FUNC.finditer(clean))
    for i, m in enumerate(starts):
        end = starts[i + 1].start() if i + 1 < len(starts) else len(clean)
        body = clean[m.end():end]
        bases = set(re.findall(r"\(\s*(?:\(\s*(?:unsigned\s+)?char\s*\*\s*\)\s*)?\b([A-Za-z_]\w*)\s*\+\s*(?:0x[0-9A-Fa-f]+|\d+)\s*\)", body))
        for var in sorted(bases | set(NAMES)):
            if declared_as_bytes(m.group(2), body, var) and is_moby(body, var, m.group(1)):
                found[var].append(m.group(1))
    return found


def main():
    args = sys.argv[1:]
    write, listing = "--write" in args, "--list" in args
    paths = [a for a in args if not a.startswith("--")]
    skip = set(",".join(a[7:] for a in args if a.startswith("--skip=")).split(",")) - {""}
    for name, others in pe.load_aliases().items():
        if name in skip:
            skip |= set(others)
    if not paths:
        out = subprocess.run(["git", "ls-files", "src/*.c"], cwd=ROOT, capture_output=True, text=True).stdout
        paths = [p for p in out.split() if not p.startswith(("src/data/", "src/libgcc/", "src/versions/"))]
    total = 0
    for rel in paths:
        found = {var: [f for f in funcs if f not in skip]
                 for var, funcs in candidates((ROOT / rel).read_text(errors="surrogateescape")).items()}
        found = {var: funcs for var, funcs in found.items() if funcs}
        for var, funcs in found.items():
            total += len(funcs)
            if listing:
                print(f"{rel}: {var}: {','.join(funcs)}"[:150])
            if write:
                run = subprocess.run([sys.executable, str(HERE / "rawstruct.py"), rel, f"--func={','.join(funcs)}",
                                      f"--var={var}", "--struct=Moby"], cwd=ROOT, capture_output=True, text=True)
                if run.returncode:
                    print(f"{rel}: {var}: rawstruct stopped: {run.stderr.strip()[-100:]}")
    print(f"{'converted' if write else 'would convert'} {total} variables")


if __name__ == "__main__":
    main()
