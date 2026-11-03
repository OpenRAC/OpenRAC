#!/usr/bin/env python3
"""
Record the field layout of a struct, so a block of data can be generated with its real type.

  python3 tools/data_types.py learn "StashBlock:structs/misc.h" ...   # needs clang; writes config/data_types.tsv
  python3 tools/data_types.py show StashBlock

A generated initialiser must print each field in the right form: a float field cannot take a hex word. The table holds
types and offsets only, so a build does not need clang.

config/data_types.tsv: type, header, size, then fields as offset:kind:size. Kinds: i integer, f float, p pointer, q quad.
"""
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TABLE = ROOT / "config/data_types.tsv"
HEADER = "# Field layouts for data with a real type (tools/data_types.py). Types and offsets only.\n# type\theader\tsize\tfields\n"
SCALARS = {"char": 1, "signed char": 1, "unsigned char": 1, "u8": 1, "s8": 1, "short": 2, "unsigned short": 2, "u16": 2,
           "s16": 2, "int": 4, "unsigned int": 4, "s32": 4, "u32": 4, "long": 8, "unsigned long": 8, "s64": 8, "u64": 8}
FLOATS = {"float": 4, "f32": 4}
QUADS = {"u128": 16, "VecQuad": 16, "long long": 16, "unsigned long long": 16}


def load():
    """{type: (header, size, [(offset, kind, size)])}"""
    if not TABLE.exists():
        return {}
    types = {}
    for line in TABLE.read_text().splitlines():
        if line and not line.startswith("#"):
            name, header, size, fields = line.split("\t")
            parsed = [tuple(f.split(":")) for f in fields.split(",")]
            types[name] = (header, int(size), [(int(o), k, int(s)) for o, k, s in parsed])
    return types


def save(types):
    rows = []
    for name, (header, size, fields) in sorted(types.items()):
        rows.append("\t".join((name, header, str(size), ",".join(f"{o}:{k}:{s}" for o, k, s in fields))) + "\n")
    TABLE.write_text(HEADER + "".join(rows))


def parse_records(dump):
    """{record type: (size, [(offset, depth, type, name)])} from clang's record layout dump."""
    records = {}
    for chunk in dump.split("*** Dumping AST Record Layout")[1:]:
        lines = [re.match(r"\s*(\d+)(?::\S+)? \|( *)(.*)$", line) for line in chunk.splitlines()]
        lines = [m for m in lines if m]
        size = re.search(r"sizeof=(\d+)", chunk)
        if not lines or not size:
            continue
        rows = []
        for m in lines[1:]:
            typed = re.match(r"(.*?)\s*(\w+)$", m.group(3))
            if typed:
                rows.append((int(m.group(1)), (len(m.group(2)) - 1) // 2, typed.group(1), typed.group(2)))
        records[lines[0].group(3).strip()] = (int(size.group(1)), rows)
    return records


def base_type(text):
    return re.sub(r"\b(?:const|volatile)\b", "", text).strip()


def flatten(rows, start, depth, base, records, out):
    """Add the leaves of the rows at this depth. Returns the index after them. Raises ValueError on what it cannot do."""
    i = start
    while i < len(rows) and rows[i][1] >= depth:
        offset, level, typ, _ = rows[i]
        typ = base_type(typ)
        children = i + 1 < len(rows) and rows[i + 1][1] > level
        if typ.startswith("union") or re.search(r"^union\b", typ):
            raise ValueError(f"a union: {typ}")
        if children and "[" not in typ:
            i = flatten(rows, i + 1, level + 1, base, records, out)
            continue
        array = re.fullmatch(r"(.*?)\s*\[(\d+)\]", typ)
        count = int(array.group(2)) if array else 1
        element = base_type(array.group(1)) if array else typ
        for n in range(count):
            leaf(element, base + offset, n, records, out)
        i += 1
    return i


def leaf(element, at, n, records, out):
    if "*" in element:
        out.append((at + n * 4, "p", 4))
    elif element in FLOATS:
        out.append((at + n * 4, "f", 4))
    elif element in SCALARS:
        size = SCALARS[element]
        out.append((at + n * size, "i", size))
    elif element in QUADS:
        out.append((at + n * 16, "q", 16))
    else:
        name = next((r for r in records if r in (element, f"struct {element}") or r.endswith(f" {element}")), None)
        if name is None:
            raise ValueError(f"unknown type: {element}")
        size, rows = records[name]
        flatten(rows, 0, 1, at + n * size, records, out)


def learn(spec):
    name, header = spec.split(":")
    sys.path.insert(0, str(ROOT / "tools/tidy"))
    import clast
    clast.prep_includes()
    source = f'#include "common.h"\n#include "{header}"\nstruct __probe {{ {name} a; }};\n'
    with tempfile.TemporaryDirectory() as folder:
        path = Path(folder) / "v.c"
        path.write_text(clast.ee(source))
        run = subprocess.run(["clang", "--target=mips64el-linux-gnuabin32", "-std=gnu89", "-fsyntax-only", "-w", "-I", str(clast.INC_T),
                              "-include", str(clast.PRELUDE), "-Xclang", "-fdump-record-layouts-complete", "-Xclang",
                              "-fdump-record-layouts", str(path)], capture_output=True, text=True)
    records = parse_records(clast.unee(run.stdout))
    probe = records.get("struct __probe")
    if not probe:
        raise ValueError(f"clang found no layout: {run.stderr.strip()[-120:]}")
    out = []
    flatten(probe[1], 0, 1, 0, records, out)
    return header, probe[0], out


def main():
    command = (sys.argv[1:] or [""])[0]
    if command == "learn":
        types = load()
        for spec in sys.argv[2:]:
            try:
                types[spec.split(":")[0]] = learn(spec)
                print(f"{spec}: {len(types[spec.split(':')[0]][2])} fields")
            except ValueError as problem:
                print(f"{spec}: skipped: {problem}")
        save(types)
    elif command == "show":
        print(load().get(sys.argv[2]))
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main()
