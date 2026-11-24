#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Checks mp/wide.json (rooms past 8 players, host/mp_wide.c) against the host C.

mp/wide.json names every instruction of the multiplayer executable (PAL) that indexes one of the
game's 8-wide per-player tables, and hostgen translates each through host/mp_wide.c. The table ids
are their places in wide.json's "tables", which must be the order of mp_wide.c's WIDE_TABLES; each
site's kind must be one hostgen knows (port/tools/hostgen/asmc.py parse_wide).

The sites come from a dataflow over each function of the executable and of level 39 (an address
formed from a table's base plus an index, or from the settings object's pointer at 0x00241640),
with the loop bounds reviewed by hand; the notes are with the port's planning notes
(rac3-mp/wide-rooms.md).

    python3 port/game/rac3-pal/mp/wide.py [--check]
"""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
HOST_C = HERE.parent / "host" / "mp_wide.c"
KINDS = {"ea": 2, "value": 3, "arg": 3, "bound": 1, "span": 1}


def problems() -> list[str]:
    table = json.loads((HERE / "wide.json").read_text())
    c_tables = re.findall(r"^\s*X\((\w+),", HOST_C.read_text(), re.M)
    out = []
    if table["tables"] != c_tables:
        out.append(f"wide.json tables {table['tables']} != mp_wide.c WIDE_TABLES {c_tables}")
    names = set(table["tables"])
    for program, rows in table["sites"].items():
        if program != "core" and not program.isdigit():
            out.append(f"program {program!r} is neither core nor a level")
        for address, text in rows.items():
            if not re.fullmatch(r"[0-9A-F]{8}", address):
                out.append(f"{program} {address}: not an address")
            for what in text.split(";"):
                words = what.split()
                if not words or words[0] not in KINDS or len(words) != KINDS[words[0]]:
                    out.append(f"{program} {address}: {what!r} is no site")
                elif len(words) > 1 and words[1] not in names:
                    out.append(f"{program} {address}: no table {words[1]}")
    return out


def main() -> int:
    found = problems()
    for p in found:
        print(f"wide.py: {p}", file=sys.stderr)
    if not found and "--check" not in sys.argv:
        print("wide.py: mp/wide.json agrees with host/mp_wide.c")
    return 1 if found else 0


if __name__ == "__main__":
    sys.exit(main())
