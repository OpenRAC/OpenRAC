#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""one_function: two C functions at one game address.

    python3 port/tools/hostgen/one_function.py <gen dir> --allow port/game/<id>/checks/two_functions_allow.tsv [--write]

The runtime keeps the first function a table gives for an address and only warns about the
other (guest.cpp, "two functions at ... keeping the first"). A release view that misses a
definition (rac2-decomp's readable names, #538) puts v1.01's C at the release's address beside
the release's own, and the game runs the wrong one. This reads hostgen's functions.c and fails on
any address with two different functions that the allow list does not name. --write writes the
list from what hostgen made (after a checked table rebuild only).
"""

from __future__ import annotations

import argparse
import re
import sys
from collections import defaultdict
from pathlib import Path

TABLE = re.compile(r"static const openrac_fn_entry k_(exe|overlay_\d+)\[\]")
ENTRY = re.compile(r'\{0x([0-9A-Fa-f]+)u, \(openrac_host_fn\)(\w+), "\w+"\}')


def shared_places(functions_c: str) -> dict[tuple[str, str], list[str]]:
    """(table, address) -> the host functions there, where there is more than one."""
    places: dict[tuple[str, str], list[str]] = defaultdict(list)
    table = None
    for line in functions_c.splitlines():
        if m := TABLE.search(line):
            table = m.group(1)
        for e in ENTRY.finditer(line):
            fns = places[(table, e.group(1).upper())]
            if e.group(2) not in fns:
                fns.append(e.group(2))
    return {k: v for k, v in places.items() if len(v) > 1}


def read_allow(path: Path) -> set[tuple[str, str]]:
    out = set()
    if path.is_file():
        for line in path.read_text().splitlines():
            if line.strip() and not line.startswith("#"):
                table, address = line.split("\t")[:2]
                out.add((table, address.upper()))
    return out


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("gen", type=Path)
    ap.add_argument("--allow", type=Path, required=True)
    ap.add_argument("--write", action="store_true")
    args = ap.parse_args(argv)
    functions = args.gen / "functions.c"
    if not functions.is_file():
        print(f"one_function: no {functions} (not generated yet): skipped", file=sys.stderr)
        return 77
    shared = shared_places(functions.read_text(errors="replace"))
    if args.write:
        rows = [f"{t}\t{a}\t{' '.join(f)}" for (t, a), f in sorted(shared.items())]
        args.allow.write_text("# Addresses with two C functions, checked (one_function.py --write).\n"
                              "# table\taddress\tfunctions\n" + "".join(r + "\n" for r in rows))
        return 0
    allowed = read_allow(args.allow)
    new = {k: v for k, v in shared.items() if k not in allowed}
    for (table, address), fns in sorted(new.items())[:40]:
        print(f"one_function: {table} 0x{address}: {', '.join(fns)}", file=sys.stderr)
    if new:
        print(f"one_function: {len(new)} address(es) with two functions not in {args.allow}; the game "
              "keeps the first. A release table that misses a definition does this (release.py).",
              file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
