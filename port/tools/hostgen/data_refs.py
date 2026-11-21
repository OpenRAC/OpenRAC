#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""The functions the game reaches through its data, as a table the build checks without the disc.

    python3 port/tools/hostgen/data_refs.py --game port/game/rac2-pal/hostgen.json \\
        --gen port/build/release/games/rac2-pal/gen --image <disc image> \\
        --out port/game/rac2-pal/checks/data_refs.tsv

A moby class's update, a menu page's callbacks, a state handler: the game calls them through an
address held in its data, so no call in C shows them. When such a function has no C, the call
finds a stub that does nothing, or no function at all ("call to code address X, which no
function of the port has"), and only play shows it.

This reads every program of the disc (the executable and each level, retail.py) and lists each
data word that holds the address where a function starts (retail.Retail.pointed: a jump table's
words are left out). It writes addresses only, no bytes of the disc. hostgen (hostgen.json
"data_refs", data_guard) then checks each row with no disc: the program must have a function
with C there, or a row of "data_allow" says why not.

Columns: the place (boot:ADDR for the executable, NN:ADDR for level NN) and the name the port has
there now (for the reader; the check goes by the place). A word that holds an address where the
port has no function at all is left out: most are a switch's cases or numbers that look like code
addresses (--unknown lists them for a person to judge).
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

import no_c
import retail

COLUMNS = ["place", "name"]


def place_text(level: int, addr: int) -> str:
    return f"{'boot' if level < 0 else f'{level:02d}'}:{addr:08X}"


def read(path: Path) -> list[tuple[int, int]]:
    """The table's places as (program, address): -1 the executable, n level n."""
    out = []
    for line in path.read_text().splitlines():
        if not line.strip() or line.startswith("#") or line.startswith("place\t"):
            continue
        where, _, addr = line.split("\t")[0].partition(":")
        out.append((-1 if where == "boot" else int(where), int(addr, 16)))
    return out


def rows(memory: retail.Retail, progs: dict[int, dict[int, str]]) -> list[list[str]]:
    """One row per function start a data word holds, in each program that holds it."""
    maps, _starts = no_c.program_maps(progs, memory)
    known = {(h, a) for h, m in progs.items() for a in m}
    pointed = memory.pointed(set(memory.levels), known)
    held: dict[tuple[int, int], int] = {}
    for level in [-1] + sorted(memory.levels):
        for part in memory.exe if level < 0 else memory.levels[level]:
            if part.code:
                continue
            for k in range(len(part.data) // 4):
                w = int.from_bytes(part.data[4 * k:4 * k + 4], "little")
                home = -1 if w < memory.low else level
                if (home, w) in pointed and (home, w) not in held:
                    held[(home, w)] = part.base + 4 * k
    out = []
    for (level, addr), at in sorted(held.items()):
        name = maps.get(level, {}).get(addr, "-")
        out.append([place_text(level, addr), name, f"{at:08X}"])
    return out


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--game", type=Path, required=True)
    ap.add_argument("--gen", type=Path, required=True, help="hostgen's output (its functions.c)")
    ap.add_argument("--image", type=Path, help="the disc image")
    ap.add_argument("--exe", type=Path, help="the retail executable, without --image")
    ap.add_argument("--dump", type=Path, help="with --exe: the level dumps")
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--unknown", action="store_true", help="print the words that hold no function's address")
    args = ap.parse_args(argv)
    cfg = no_c.configure(args.game)
    low = (cfg.get("relocate") or [0x15F000])[0]
    if args.image:
        memory = retail.Retail.from_image(args.image, low)
    elif args.exe:
        memory = retail.Retail.from_files(args.exe, args.dump, low)
    else:
        print("data_refs: give the disc image or the executable", file=sys.stderr)
        return 1
    table = rows(memory, no_c.read_functions_c(args.gen / "functions.c"))
    head = [f"# {cfg.get('title', cfg.get('id', ''))}: the functions the game reaches through an address in its data.",
            "# Written by port/tools/hostgen/data_refs.py from the disc (addresses only); hostgen's data_guard",
            "# checks that each place has a function with C.", "\t".join(COLUMNS)]
    known = [r[:2] for r in table if r[1] != "-"]
    args.out.write_text("\n".join(head + ["\t".join(r) for r in known]) + "\n")
    if args.unknown:
        for place, _name, at in (r for r in table if r[1] == "-"):
            print(f"{place}: no function of the port starts here; held at {at}")
    print(f"data_refs: {len(known)} places, {len(table) - len(known)} words with no function")
    return 0


if __name__ == "__main__":
    sys.exit(main())
