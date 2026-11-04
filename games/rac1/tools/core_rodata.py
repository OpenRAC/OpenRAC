#!/usr/bin/env python3
"""
The core_rdata pieces that compiled objects provide (config/core_rodata.txt).

  python tools/core_rodata.py labels   # labels to cut, for split_data_s.py
  python tools/core_rodata.py ld       # the .core_rdata section's contents

split_data_s.py starts a new piece at every cut label, so with labels
L1..Ln in address order the pieces are build-sn/core_rdata_1..n+1, and
piece k+1 follows label k. An object's .rodata goes after the piece that
ends at its first label; the pieces between its own labels are empty.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def sections() -> list[tuple[str, str, list[str]]]:
    """Object, input section, and labels; old rows default to .rodata."""
    out = []
    for line in (ROOT / "config/core_rodata.txt").read_text().splitlines():
        fields = line.split("#")[0].split()
        if fields:
            match = re.fullmatch(r"(.+\.o)(?:\(([^()]+)\))?", fields[0])
            if not match:
                raise ValueError(f"invalid core rodata object: {fields[0]}")
            out.append((match[1], match[2] or ".rodata", fields[1:]))
    return out


def entries() -> list[tuple[str, list[str]]]:
    return [(obj, labels) for obj, _section, labels in sections()]


def main() -> None:
    import data_in_c  # it reads this file's table, so it comes in only here

    rows = sorted(((int(lab.rsplit("_", 1)[1], 16), lab, f"{obj}({section})" if i == 0 else None)
                   for obj, section, labs in sections() for i, lab in enumerate(labs)))
    # Blocks in src/data/ fill the rest of core_rdata the same way.
    rows = sorted(rows + [(data_in_c.address(label), label, f"{obj}({section})")
                          for label, obj, section in data_in_c.provided("core_rdata")])
    if sys.argv[1:] == ["labels"]:
        inside = [f"+{inner}" for inner in data_in_c.interior() if data_in_c.segment_of(inner) == "core_rdata"]
        print(" ".join([lab for _, lab, _ in rows] + inside))
    elif sys.argv[1:] == ["ld"]:
        print("    build-sn/core_rdata_1.o(.rodata)")
        for k, (_, lab, obj) in enumerate(rows, 2):
            if obj:
                print(f"    {obj}")
            for line in data_in_c.inner_symbols(lab):
                print(line)
            print(f"    build-sn/core_rdata_{k}.o(.rodata)")
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main()
