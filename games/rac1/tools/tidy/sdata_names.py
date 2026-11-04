#!/usr/bin/env python3
"""
Use the readable name inside SDATA().

  python3 tools/tidy/sdata_names.py            # dry run: show counts
  python3 tools/tidy/sdata_names.py --write

`extern float jump_speed SDATA(D_L17_00162108);` becomes `extern float jump_speed SDATA(jump_speed);`, when names.h maps
that name to exactly that address. SDATA expands its argument first, so the label stays the same.
"""
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import prune_externs as pe  # noqa: E402

ROOT = pe.ROOT
DECLARATION = re.compile(r"(\bextern\b[^;\n]*?\b(\w+)\s+SDATA\()((?:D)_(?:L\d+_)?[0-9A-Fa-f]+)(\)\s*;)")


def rewrite(text, aliases):
    """(new text, number changed)."""
    count = [0]

    def one(m):
        name, symbol = m.group(2), m.group(3)
        if name != symbol and aliases.get(name) == {symbol}:
            count[0] += 1
            return f"{m.group(1)}{name}{m.group(4)}"
        return m.group(0)

    return DECLARATION.sub(one, text), count[0]


def main():
    write = "--write" in sys.argv[1:]
    aliases = {name: set(others) for name, others in pe.load_aliases().items()}
    out = subprocess.run(["git", "ls-files", "src/*.c"], cwd=ROOT, capture_output=True, text=True).stdout
    total = 0
    for rel in out.split():
        path = ROOT / rel
        text = path.read_text(errors="surrogateescape")
        new, count = rewrite(text, aliases)
        total += count
        if write and count:
            path.write_text(new, errors="surrogateescape")
    print(f"{'changed' if write else 'would change'} {total} declarations")


if __name__ == "__main__":
    main()
