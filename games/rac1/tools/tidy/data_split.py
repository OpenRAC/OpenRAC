#!/usr/bin/env python3
"""
Split data blocks at the addresses the code uses inside them.

  python3 tools/tidy/data_split.py            # dry run: show counts
  python3 tools/tidy/data_split.py --write

The disassembly only has a label where it saw a reference, so a block can hide many variables: `pad_globals_base` holds
g_controller_state and pad_held. An address the code names, or reaches as base + constant, starts a block of its own, if
GCC could have put it there: on 8 bytes, or a single word on 4. Run tools/gate_loop.py afterwards.
"""
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import data_in_c as d  # noqa: E402

ADDRESS = re.compile(r"\bD_([0-9A-Fa-f]{8})\b")
BASE = re.compile(r"(?<![\w.>])([A-Za-z_]\w*)\s*\+\s*(0x[0-9A-Fa-f]+|\d+)\b(?!\s*[*/%])")


def symbols():
    text = (ROOT / "include/names/executable.h").read_text(errors="replace")
    return {name: int(value[2:], 16) for name, value in re.findall(r"^#define\s+(\w+)\s+(D_[0-9A-Fa-f]{8})\b", text, re.M)}


def used_addresses(texts, defined):
    """Addresses the code names directly, or reaches from a name plus a constant."""
    used = set()
    for text in texts:
        code = re.sub(r"/\*.*?\*/|//[^\n]*", " ", text, flags=re.S)
        used.update(int(m, 16) for m in ADDRESS.findall(code))
        words = set(re.findall(r"\b\w+\b", code))
        used.update(defined[w] for w in words & defined.keys())
        for name, constant in BASE.findall(code):
            base = defined.get(name)
            if base is None:
                m = ADDRESS.fullmatch(name)
                base = int(m.group(1), 16) if m else None
            if base is not None:
                used.add(base + int(constant, 0))
    return used


def split_points(start, size, used):
    """Addresses inside a block of this extent that can start a block of their own."""
    inside = sorted(a for a in used if start < a < start + size and a % 4 == 0)
    edges = inside + [start + size]
    chosen = []
    for at, following in zip(inside, edges[1:]):
        if at % 8 == 0 or following - at == 4:
            chosen.append(at)
    return chosen


def line_starts(body):
    """Addresses where a line of the disassembly starts. A cut can only go there, not inside a string."""
    return {at for at in map(lambda line: d.split_data_s.first_address([line]), body.splitlines()) if at is not None}


def block_size(body):
    parsed = d.parse(body)
    if parsed is None or parsed[0] == "char":
        return 0
    return len(parsed[1]) * {"u32": 4, "u16": 2, "u8": 1}[parsed[0]]


def main():
    out = subprocess.run(["git", "ls-files", "src/*.c", "include/*.h"], cwd=ROOT, capture_output=True, text=True).stdout
    paths = [p for p in out.split() if not p.startswith(("src/data/", "include/names", "src/libgcc"))]
    used = used_addresses([(ROOT / p).read_text(errors="replace") for p in paths], symbols())
    blocks = d.all_blocks()
    table = {row[0]: row for row in d.rows()}
    added = []
    for label, element, source in list(table.values()):
        size = block_size(blocks[label])
        starts = line_starts(blocks[label])
        for at in split_points(d.address(label), size, used & starts):
            added.append((f"D_{at:08X}", "u32", source))
    print(f"{len(added)} new blocks from {len(used)} addresses the code uses")
    if "--write" in sys.argv[1:]:
        for row in added:
            table.setdefault(row[0], row)
        d.write_rows(list(table.values()))
        d.complete()


if __name__ == "__main__":
    main()
