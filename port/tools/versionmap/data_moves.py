#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""How a release moved the primary's data, from versionmap's map, kept in the repo (no disc then).

    data_moves.py write MAP OUT     the map's data pairs as runs of one move (data_moves.tsv)
    data_moves.py check OUT ADDR... where the release has each address (program:address)

versionmap.py's map pairs each data address of the primary with the release's (from the code
that forms it). Consecutive pairs that moved by as much make one run: an address from a run's
first to its last pair moved by that much, and parity.py takes that as evidence. The file holds
addresses only, never the disc's bytes.
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path


def runs(vmap: dict) -> list[tuple[str, int, int, int, int]]:
    """(program, first, last, move, pairs) for each run of pairs that moved alike."""
    out = []
    for program, part in sorted(vmap["programs"].items()):
        run = None
        for key in sorted(part.get("data", {}), key=lambda k: int(k, 16)):
            a, b = int(key, 16), int(part["data"][key], 16)
            if run and run[3] == b - a:
                run[2], run[4] = a, run[4] + 1
                continue
            if run:
                out.append(tuple(run))
            run = [program, a, a, b - a, 1]
        if run:
            out.append(tuple(run))
    return out


def write(vmap: dict, about: str) -> str:
    lines = [f"# {about}: how the release moved the primary's data (port/tools/versionmap/data_moves.py",
             "# write, from versionmap's map). An address from a run's first pair to its last moved by its move.",
             "# program\tfirst\tlast\tmove\tpairs"]
    for program, first, last, move, pairs in runs(vmap):
        lines.append(f"{program}\t{first:08X}\t{last:08X}\t{move:+#x}\t{pairs}")
    return "\n".join(lines) + "\n"


def read(path: Path) -> dict[str, list[tuple[int, int, int]]]:
    """program ("boot", "level:NN") -> [(first, last, move)], sorted."""
    out: dict[str, list[tuple[int, int, int]]] = {}
    if path.is_file():
        for line in path.read_text().splitlines():
            cols = line.split("\t")
            if line.startswith("#") or len(cols) < 4:
                continue
            out.setdefault(cols[0], []).append((int(cols[1], 16), int(cols[2], 16), int(cols[3], 16)))
    for rows in out.values():
        rows.sort()
    return out


def moved(table: dict[str, list[tuple[int, int, int]]], program: str, address: int) -> int | None:
    """The release's address for ADDRESS in PROGRAM, when a run holds it."""
    for first, last, move in table.get(program, ()):
        if first <= address <= last:
            return address + move
    return None


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    w = sub.add_parser("write")
    w.add_argument("map", type=Path)
    w.add_argument("out", type=Path)
    c = sub.add_parser("check")
    c.add_argument("table", type=Path)
    c.add_argument("addresses", nargs="+")
    args = ap.parse_args(argv)
    if args.cmd == "write":
        vmap = json.loads(args.map.read_text())
        args.out.write_text(write(vmap, f"{vmap.get('from', '?')} to {vmap.get('to', '?')}"))
        return 0
    table = read(args.table)
    for item in args.addresses:
        program, _, address = item.rpartition(":")
        got = moved(table, program or "boot", int(address, 16))
        print(f"{item}\t{'-' if got is None else f'{got:08X}'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
