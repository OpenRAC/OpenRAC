#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Up Your Arsenal's call_copies.tsv: the calls that differ between the copies of a folded function.

copycalls.py writes this table for Going Commando from its extracted levels. Up Your Arsenal's
catalogue (config/levels/functions.tsv) folds copies by a fingerprint that leaves address fields
out, so one name covers copies whose jal at the same word calls different functions:
func_L03_0034B248, an emitter's tick, makes its particle through func_L03_00461E88 in levels 3, 5,
8, 13, 22 and 24, while its copies in the vid-comics (30 to 36) call func_L30_00442950, whose
particle keeps its emitter at +0x30. Translated once, every vid-comic ran level 3's maker, whose
particle (its emitter at +0x20) the vid-comic's callback overwrites; the next tick called through
the overwritten words and faulted. hostgen calls such a jal through openrac_call_in_copy (the
running copy's own target), as for Going Commando (hostgen.json "call_copies"). One row per caller
and callee name, as copycalls.py's:

    caller  callee  home  canon  size  own  exe

home, canon: a level holding the caller and that call, and the caller's place there (the first such
level below the multiplayer ones); size: the caller's, in bytes; own: the callee's place in home;
exe 0; words: the indices of the jals where a copy calls it (comma-separated), for hostgen's check
that a call's other callees there read no more registers than the named one
(call_copies_same_shape). The front end's functions (kind frontbin) are left out: their C runs in the front end too,
where no level copy is loaded. So are wrappers of 64 bytes or less (variant_callees.py's): the
fingerprint folds every wrapper of one shape, so a level's place of the name is often another
routine than the one the code calling it by that name reaches there (func_L01_004127C0's copy in
Annihilation Nation); the code written for the name keeps its callee. Re-run after a catalogue change (a decomp pin bump):

    python3 port/tools/hostgen/call_copies_rac3.py --source games/rac3/decomp \\
        --split port/game/rac3-pal/tables/split_places.tsv --out port/game/rac3-pal/tables/call_copies.tsv
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from copycalls import jal_target, words  # noqa: E402
from variant_callees import MAX_WRAPPER, apply_split, level_programs  # noqa: E402

HEADER = "# caller\tcallee\thome\tcanon\tsize\town\texe\twords -- port/tools/hostgen/call_copies_rac3.py\n"
MULTIPLAYER = 39  # the multiplayer menus and maps: another executable's levels


def catalogue(path: Path) -> tuple[list[tuple[str, int, list[tuple[int, int]]]], dict[str, str]]:
    table, kinds = [], {}
    for line in path.read_text().splitlines():
        cols = line.split("\t")
        if line.startswith("#") or len(cols) < 7:
            continue
        places = [(int(p[:2]), int(p[3:], 16)) for p in cols[6].split(",") if p]
        table.append((cols[0], int(cols[2]), places))
        kinds[cols[0]] = cols[1]
    return table, kinds


def rows(source: Path, split: Path | None = None) -> list[tuple[str, str, int, int, int, int, tuple[int, ...]]]:
    base, kinds = catalogue(source / "config/levels/functions.tsv")
    table = apply_split(base, split)
    names = {place: name for name, _, places in table for place in places}
    programs = level_programs(source / "baserom/levels")
    out = []
    for caller, size, places in table:
        if kinds.get(caller) == "frontbin":
            continue
        size &= ~3
        copies = [(level, address, words(programs[level], address, size))
                  for level, address in sorted(places) if level in programs]
        copies = [c for c in copies if c[2] is not None]
        if len(copies) < 2 or size <= MAX_WRAPPER:
            continue
        found: dict[str, tuple[int, int, int]] = {}
        at: dict[str, list[int]] = {}
        for i in range(size // 4):
            targets = [(level, address, jal_target(w[i])) for level, address, w in copies]
            if any(t is None for _, _, t in targets):
                continue
            called: dict[str, tuple[int, int, int]] = {}
            for level, address, t in targets:
                name = names.get((level, t))
                if name is not None and (name not in called or called[name][0] >= MULTIPLAYER):
                    called[name] = (level, address, t)
            if len(set(names.get((level, t), f"?{t:X}") for level, _, t in targets)) < 2:
                continue
            for name, place in called.items():
                if place[0] < MULTIPLAYER:
                    found.setdefault(name, place)
                    at.setdefault(name, []).append(i)
        for name, (level, address, target) in sorted(found.items()):
            out.append((caller, name, level, address, size, target, tuple(at[name])))
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--source", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--split", type=Path, help="the game's tables/split_places.tsv, applied to the catalogue")
    args = ap.parse_args()
    found = rows(args.source, args.split)
    text = HEADER + "".join(f"{c}\t{e}\t{lv}\t{pl:08X}\t{sz}\t{t:08X}\t00000000\t{','.join(map(str, w))}\n"
                            for c, e, lv, pl, sz, t, w in found)
    args.out.write_text(text)
    print(f"call_copies: {len({r[0] for r in found})} callers, {len(found)} rows", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
