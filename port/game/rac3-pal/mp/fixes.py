#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""The multiplayer maps' stock-bug fixes (host/mp_fixes.c): which of each map's functions they wrap
or call, at each map's place.

Up Your Arsenal's multiplayer has bugs of its own that show up in online play (a gadget moby whose
class the map never loaded, credit for hitting a player already dead, shooting and jumping while
dead, a flux rifle light left lit for good, remote players dropped for a late packet, ...).
host/mp_fixes.c fixes them in C around the game's own functions. Each map carries its own copy of
the engine (mp/maps.py), so each map's copy of each of those functions is wrapped: this writes,
from rac3-decomp's catalogue (config/levels/functions.tsv), keyed by level 39's place of each:

- the rows between the markers in mp/libraries.tsv, one per map and wrapped function;
- host/mp_fixes.h, one MP_FIX(map, ...) line per map with each function's place there, in
  FIXED's order (host/mp_fixes.c expands it).

    python3 port/game/rac3-pal/mp/fixes.py [--source games/rac3/decomp] [--check]
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
MAPS = range(40, 54)

# Level 39's place of each function, the library, the row's name (the map's number follows), the
# port value ("wrap": host/mp_fixes.c stands in front of it; "ref": it only calls it), what it is.
FIXED = [
    ("0063FDA8", "gameplay", "rac3HeroEquipGadgetMap", "wrap",
     "the hero's gadget equip (creates the gadget's mobys)"),
    ("005AA140", "gameplay", "rac3CreateMobyMap", "wrap", "CreateMoby (frontbin's func_003C24B0)"),
    ("005AA510", "", "", "ref", "DeleteMoby (frontbin's func_003C2880)"),
    ("00626D38", "gameplay", "rac3PlayerHitMap", "wrap", "a player's hit by a damaging moby"),
    ("00626998", "gameplay", "rac3WhoHitMeMap", "wrap", "the hit's credit to the player who hit"),
    ("006606E0", "gameplay", "rac3WeaponUpdateMap", "wrap", "a player's weapon update (firing)"),
    ("005C2F50", "gameplay", "rac3PadPressedMap", "wrap", "a pad's pressed buttons"),
    ("0054F4E0", "gameplay", "rac3FluxFireMap", "wrap", "the flux rifle's shot (its muzzle light)"),
    ("0059ECB8", "", "", "ref", "a point light's delete (func_L01_00424D88)"),
    ("00550D48", "gameplay", "rac3ShotCollisionMap", "wrap", "a sniper shot's collision"),
    ("00580220", "gameplay", "rac3CollLineMap", "wrap", "the collision line test (frontbin's func_003D3CB0)"),
    ("006481F8", "net", "rac3PlayerStateFreshMap", "wrap",
     "whether a remote player's state is recent (turrets wait on it)"),
    ("00666160", "net", "rac3PlayerNetApplyMap", "wrap",
     "a remote player's net state applied (lags out a late player)"),
    ("0065F4B8", "net", "rac3LocalPlayerUpdateMap", "wrap",
     "the local player's update (stalls on a silent host)"),
    ("00656F28", "gameplay", "rac3PlayerUpdateMap", "wrap", "a player's update (deaths, the base)"),
    ("00672DD8", "net", "rac3GadgetEventMap", "wrap", "a gadget event's dispatch (shots, at a time)"),
    ("005C42A0", "net", "rac3PadUpdateMap", "wrap",
     "the pads' update the main loop runs each frame (func_L01_00461818)"),
]

BEGIN = "# --- the stock-bug fixes' rows: written by mp/fixes.py, do not edit by hand ---"
END = "# --- end of the stock-bug fixes' rows ---"
MAPS_BEGIN = "# --- the maps' rows: written by mp/maps.py, do not edit by hand ---"


def catalogue(source: Path) -> dict[str, dict[int, str]]:
    """Level 39's place of each catalogue row -> {level: place} of that row."""
    out = {}
    for line in (source / "config/levels/functions.tsv").read_text().splitlines():
        cols = line.split("\t")
        if line.startswith("#") or len(cols) < 7:
            continue
        places: dict[int, str] = {}
        for item in cols[6].split(","):
            level, _, address = item.partition(":")
            if address and level.isdigit():
                places.setdefault(int(level), address)
        if 39 in places:
            out[places[39]] = places
    return out


def build(source: Path) -> tuple[list[str], list[str]]:
    rows = catalogue(source)
    lines, macros = [], []
    for m in MAPS:
        places = []
        for l39, lib, name, port, what in FIXED:
            place = rows.get(l39, {}).get(m)
            if place is None:
                raise SystemExit(f"fixes.py: level 39's {l39} has no place in map {m} in the catalogue")
            places.append(place)
            if port != "ref":
                lines.append(f"func_L{m}_{place}\t{lib}\t{name}{m}\t{port}\tmap {m}'s copy of {what} "
                             f"(level 39's {l39}): a stock bug fixed (host/mp_fixes.c)")
        macros.append(f"MP_FIX({m}, " + ", ".join(places) + ")")
    return lines, macros


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--source", type=Path, default=ROOT / "games/rac3/decomp")
    ap.add_argument("--check", action="store_true", help="fail if a written file would change")
    args = ap.parse_args()
    lines, macros = build(args.source)
    outputs = {}

    # The rows go just before the maps' block (mp/maps.py keeps what is outside its markers).
    lib_path = HERE / "libraries.tsv"
    text = lib_path.read_text()
    if BEGIN in text:
        head, _, rest = text.partition(BEGIN)
        _, _, tail = rest.partition(END + "\n")
    elif MAPS_BEGIN in text:
        head, _, tail = text.partition(MAPS_BEGIN)
        tail = MAPS_BEGIN + tail
    else:
        head, tail = text, ""
    outputs[lib_path] = head + BEGIN + "\n" + "\n".join(lines) + "\n" + END + "\n" + tail

    header = HERE.parent / "host/mp_fixes.h"
    outputs[header] = (
        "/* SPDX-License-Identifier: GPL-3.0-or-later\n"
        " * Copyright (c) 2026 the OpenRAC contributors\n *\n"
        " * Written by port/game/rac3-pal/mp/fixes.py from rac3-decomp's catalogue: do not edit by hand.\n"
        " * Each multiplayer map's places of the functions mp/fixes.py FIXED lists, in its order. */\n"
        + "/* clang-format off */\n" + "\n".join(macros) + "\n/* clang-format on */\n")

    stale = [p for p, t in outputs.items() if not p.is_file() or p.read_text() != t]
    if args.check:
        for p in stale:
            print(f"fixes.py: {p.relative_to(ROOT)} is not what mp/fixes.py writes", file=sys.stderr)
        return 1 if stale else 0
    for p in stale:
        p.write_text(outputs[p])
        print(f"wrote {p.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
