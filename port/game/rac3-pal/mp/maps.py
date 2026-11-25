#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""The multiplayer maps (levels 40 to 53 on PAL) in the multiplayer executable's port: what
hostgen translates and which of each map's functions the port's host C stands for.

Each map's program is translated from its own instructions at its own addresses, as level 39's (the
menus) is: its functions are named func_L<map>_<address>. Every map carries a copy of the engine
that level 39 carries too (rac3-decomp's catalogue, config/levels/functions.tsv, gives each copy's
place in each level), so the port's host C for a map's copy is level 39's, its globals relocated
to the map's (OPENRAC_LDATA(39, ...): the runtime pairs level 39's program with the map's through
the catalogue's places, hostgen.json "level_copies").

This writes, from the catalogue and the level programs' entries:
- mp/hostgen.json: the maps as "asm" "retail" "levels" and "discover_levels", each map's entry and
  the second entry 0x484 past it as roots (the second an "inner_entries" one), and the maps for
  "level_copies";
- the rows between the markers in mp/libraries.tsv, one per map and function of CARRIED;
- host/mp_maps.h, one MP_MAP(map, ...) line per map with each CARRIED function's place there, in
  CARRIED's order (host/mp_maps.c expands it), and the MP_MAP_UI, MP_MAP_HUD and MP_MAP_VU0 lines of
  CARRIED_UI, CARRIED_HUD and CARRIED_VU0 (host/ui2d.c, host/hud.c and host/trig.c expand them).

    python3 port/game/rac3-pal/mp/maps.py [--source games/rac3/decomp] [--check]

The level programs' entries are read from rac3-decomp's baserom/levels/<NN>_*.elf.
"""
from __future__ import annotations

import argparse
import json
import re
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
MAPS = range(40, 54)
SECOND_ENTRY = 0x484  # main (0x001932A0) also calls the routine this far past a map's entry

# Level 39's function (its place), the library, the row's name (the map's number follows), the
# port value, what it is, the host C's signature. host/mp_maps.c gives the host C in this order.
CARRIED = [
    ("005FCB98", "dma", "rac3KickVif0ChainMap", "host",
     "KickVif0Chain (the catalogue's frontbin row func_003A8F90): it polled VIF0's CHCR for ever",
     "void (unsigned int)"),
    ("005A9AA0", "wad", "rac3WadDecompressMap", "host", "the WAD decompressor (func_003A0760)",
     "int (int, int)"),
    ("0060DFC8", "render", "rac3ScreenGlowCopyMap", "host",
     "the screen glow's first packet (func_L01_004DE670), left out", "int (void)"),
    ("0060E048", "render", "rac3ScreenGlowAddMap", "host",
     "the screen glow's second packet (func_L01_004DE6F0), left out", "int (void)"),
    ("0060EC68", "render", "rac3ScreenEffect2Map", "host",
     "the second screen effect (func_L01_004DF310), left out", "int (void)"),
    ("005F4690", "render", "rac3DrawTfragMap", "host", "DrawTfrag (func_L39_005F4690)", "int (void)"),
    ("005F61A0", "render", "rac3DrawTiesMap", "host", "the tie pass (func_L39_005F61A0)", "int (void)"),
    ("005EDE28", "render", "rac3DrawShrubsMap", "host", "the shrub pass (func_L39_005EDE28)",
     "int (void)"),
    ("005B25C8", "render", "rac3MobyProcMap", "host", "MobyProc (func_L39_005B25C8)",
     "int (int, int, int, int)"),
    ("005B24E8", "render", "rac3MobyDrawSideMap", "host",
     "MobyAnimProc's draw side (func_L01_00441A98): replaced", "int (int, int)"),
    ("005B2238", "render", "rac3MobyTexturesMap", "host",
     "the moby textures' DMA (func_L01_004417E8): nothing to send", "int (int)"),
    ("005B0AE0", "render", "rac3MobyChainMap", "host",
     "moby_anim_eval_chain (func_L01_00440090): the port's evaluator", "int (int, int)"),
    ("005F5720", "render", "rac3TfragLightingMap", "host",
     "the terrain's vertex lighting (func_L01_004B27B8): the extracted level carries it", "int (int)"),
    ("005F45F0", "render", "rac3TfragGifsMap", "host",
     "the terrain's texture packets (func_L01_004B1688): none to patch", "int (void)"),
    ("005F9F80", "render", "rac3TieColoursMap", "host",
     "the ties' colours (func_L01_004B6ED0): the extracted level carries them", "int (void)"),
    ("005F60E0", "render", "rac3TieGifsMap", "host",
     "the ties' texture packets (func_L01_004B3178): none to patch", "int (void)"),
    ("005EFA10", "render", "rac3ShrubLightingMap", "host",
     "the shrubs' lighting (func_L01_004A75E0): the extracted level carries it", "int (void)"),
    ("005EDD20", "render", "rac3ShrubGifsMap", "host",
     "the shrubs' texture packets (func_L01_004A58F0): none to patch", "int (void)"),
    ("005AB950", "render", "rac3MobyGifsMap", "host",
     "the mobys' texture packets (func_L01_0043AAC0): none to patch", "int (void)"),
    ("005F0588", "render", "rac3SkyDrawShellMap", "host",
     "SkyDrawShell (func_L01_004A8158): the shell drawn natively", "int (int)"),
    ("005C4768", "render", "rac3PartProcMap", "host",
     "PartProc (func_L01_00461CB8): each live particle to the window", "int (void)"),
    ("0058D140", "render", "rac3DrawWorldQuadMap", "host",
     "DrawWorldQuad (func_L01_00407E28): drawn natively", "int (int, int)"),
    ("00588A08", "render", "rac3ViewLayoutMap", "wrap",
     "the view layout (func_L01_00403168): a split-screen view's rectangle to the frame recorder",
     "void (int, int, int, int)"),
    ("00580220", "collision", "rac3CollLineMap", "wrap",
     "coll_line (func_L01_003FA5F0): its gather bounded to the scratchpad (host/coll_line.c)",
     "int (int, int, int, int, int)"),
]

# Level 39's 2D layer (the wrap rows ui2d.c's MP_MAP_UI stands for; "ref" ones it only calls).
CARRIED_UI = [
    ("00592238", "draw", "FontPrintMap", "wrap", "FontPrint (the boot program's func_0038EBC8)"),
    ("00592AC8", "draw", "FontPrintShadedMap", "wrap", "FontPrintShaded (func_0038F458)"),
    ("0058BBB0", "draw", "DrawGlyphMap", "wrap", "DrawGlyph (func_0038BA50)"),
    ("0058BD90", "draw", "DrawGlyphShadedMap", "wrap", "DrawGlyphShaded (func_0038BC30)"),
    ("0058BF70", "draw", "DrawGlyphShadowedMap", "wrap", "DrawGlyphShadowed (func_0038BE10)"),
    ("0058B868", "draw", "DrawGlyphQuadMap", "wrap", "the glyph quad (func_0038B8B0)"),
    ("0058B4B8", "draw", "DrawTurnedGlyphQuadMap", "wrap", "the turned glyph quad (func_0038B500)"),
    ("0058C678", "draw", "DrawShadedBoxMap", "wrap", "the shaded box (func_0038C628)"),
    ("0058B1B8", "draw", "DarkenPictureMap", "wrap", "the picture's darkening (func_0038B200)"),
    ("0058B2C8", "draw", "DrawFlatBoxMap", "wrap", "the flat box (func_0038B310)"),
    ("0059CFD0", "draw", "DrawHudSpriteShadedMap", "wrap", "the turned HUD sprite (func_00397680)"),
    ("00590A80", "", "", "ref", "the sine"),
    ("00590A68", "", "", "ref", "the cosine"),
    ("0059C910", "", "", "ref", "a HUD texture's TEX0"),
    ("0058C218", "draw", "DrawRotatedSpriteMap", "wrap", "the turned sprite (func_0038C0B8)"),
    ("0059D368", "draw", "DrawHudQuadTurnedMap", "wrap", "the turned HUD quad (func_00397A18)"),
    ("0058B9F8", "draw", "DrawGlyphQuadFloatMap", "wrap", "the glyph quad placed in floats"),
]

# Level 39's HUD sprites (hud.c's MP_MAP_HUD: the sprite, the gradient, then the TEX0 getter they call).
CARRIED_HUD = [
    ("0059CB38", "hud", "DrawHudSpriteMap", "wrap", "DrawHudSprite (func_L01_004202A8)"),
    ("0059CD28", "hud", "DrawHudGradientMap", "wrap", "DrawHudGradient (func_L01_00420498)"),
    ("0059C910", "", "", "ref", "a HUD texture's TEX0"),
]

# Level 39's copies of the routines that started VU0's microprogram (trig.c's MP_MAP_VU0, in its
# order: the grid's relink last, which the moby routines end in and the port does not replace).
CARRIED_VU0 = [
    ("00590A68", "rac3CosineMap", "the cosine (the boot program's func_0038D478)", "float (float)"),
    ("00590A80", "rac3SineMap", "the sine (func_0038D490)", "float (float)"),
    ("00590CD8", "rac3EulerRowsMap", "the Euler rows (func_0038D6E8)", "void (unsigned int, unsigned int)"),
    ("00590CF8", "rac3EulerRows2Map", "the Euler rows (func_0038D708)", "void (unsigned int, unsigned int)"),
    ("00591220", "rac3QuaternionRowsMap", "the quaternion rows (func_0038DBF0)",
     "void (unsigned int, unsigned int)"),
    ("00591240", "rac3QuaternionMatrixMap", "the quaternion matrix (func_0038DC10)",
     "void (unsigned int, unsigned int)"),
    ("005912A0", "rac3TurnAboutAxisMap", "the turn about an axis (func_0038DC70)",
     "void (unsigned int, unsigned int, unsigned int, float)"),
    ("005AD2B0", "rac3MobyMatrixMap", "the moby matrix (func_003C7FF0)", "void (unsigned int)"),
    ("005AD450", "rac3MobyMatrixKeptMap", "the moby matrix on its own rows (func_003C8190)",
     "void (unsigned int)"),
    ("005ECB30", "rac3RingPointsMap", "the ring of points (func_003D1DD8)", "void (unsigned int)"),
    ("005ACFD0", "", "the moby grid's relink (func_003C7D10)", ""),
]

BEGIN = "# --- the maps' rows: written by mp/maps.py, do not edit by hand ---"
END = "# --- end of the maps' rows ---"


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


def all_places(source: Path) -> tuple[dict[tuple[int, str], dict[int, str]], dict[str, dict[int, str]]]:
    """Every catalogue row's places: by (level, place) and by the row's name."""
    by_place, by_name = {}, {}
    for line in (source / "config/levels/functions.tsv").read_text().splitlines():
        cols = line.split("\t")
        if line.startswith("#") or len(cols) < 7:
            continue
        places: dict[int, str] = {}
        for item in cols[6].split(","):
            level, _, address = item.partition(":")
            if address and level.isdigit():
                places.setdefault(int(level), address)
        by_name[cols[0]] = places
        for level, address in places.items():
            by_place[(level, address)] = places
    return by_place, by_name


def map_signatures(source: Path, mp_cfg: dict) -> dict[str, str]:
    """The "asm" "signatures" a map's copies take: level 39's (this executable's hostgen.json) and
    the single-player levels' and frontbin's (the boot program's hostgen.json), which are the same
    routines' (the catalogue's rows), at each map's place. Level 39's win where both give one."""
    by_place, by_name = all_places(source)
    boot = json.loads((HERE.parent / "hostgen.json").read_text())["asm"]["signatures"]
    out: dict[str, str] = {}
    for table in (boot, mp_cfg["asm"]["signatures"]):
        for name, sig in table.items():
            m = re.fullmatch(r"func_L(\d\d)_([0-9A-F]{8})", name)
            if m:
                places = by_place.get((int(m.group(1)), m.group(2)))
            elif name.endswith("__frontbin"):
                places = by_name.get(name[: -len("__frontbin")])
            else:
                continue
            for m_ in MAPS:
                if places and m_ in places:
                    out[f"func_L{m_}_{places[m_]}"] = sig
    return out


def entry(source: Path, level: int) -> int:
    """The level program's entry, its ELF's e_entry."""
    path = next((source / "baserom/levels").glob(f"{level:02d}_*.elf"))
    head = path.read_bytes()[:0x1C]
    return struct.unpack_from("<I", head, 0x18)[0]


def build(source: Path) -> tuple[dict, list[str], list[str]]:
    rows = catalogue(source)
    entries, lines, macros = {}, [], []
    for m in MAPS:
        entries[m] = entry(source, m)
        places = []
        for l39, lib, name, port, what, sig in CARRIED:
            place = rows.get(l39, {}).get(m)
            if place is None:
                raise SystemExit(f"maps.py: level 39's {l39} has no place in map {m} in the catalogue")
            places.append(place)
            lines.append(f"func_L{m}_{place}\t{lib}\t{name}{m}\t{port}\tmap {m}'s copy of {what} "
                         f"(level 39's {l39}; host/mp_maps.c)\t{sig}")
        macros.append(f"MP_MAP({m}, " + ", ".join(places) + ")")
        ui = []
        for l39, lib, name, port, what in CARRIED_UI:
            place = rows.get(l39, {}).get(m)
            if place is None:
                raise SystemExit(f"maps.py: level 39's {l39} has no place in map {m} in the catalogue")
            ui.append(place)
            if port != "ref":
                lines.append(f"func_L{m}_{place}\t{lib}\t{name}{m}\t{port}\tmap {m}'s copy of {what}, "
                             f"wrapped as level 39's {l39} (host/ui2d.c)")
        macros.append(f"MP_MAP_UI({m}, " + ", ".join(ui) + ")")
        hud = []
        for l39, lib, name, port, what in CARRIED_HUD:
            place = rows.get(l39, {}).get(m)
            if place is None:
                raise SystemExit(f"maps.py: level 39's {l39} has no place in map {m} in the catalogue")
            hud.append(place)
            if port != "ref":
                lines.append(f"func_L{m}_{place}\t{lib}\t{name}{m}\t{port}\tmap {m}'s copy of {what}, "
                             f"wrapped as level 39's {l39} (host/hud.c)")
        macros.append(f"MP_MAP_HUD({m}, " + ", ".join(hud) + ")")
        vu0 = []
        for l39, name, what, sig in CARRIED_VU0:
            place = rows.get(l39, {}).get(m)
            if place is None:
                raise SystemExit(f"maps.py: level 39's {l39} has no place in map {m} in the catalogue")
            vu0.append(place)
            if name:
                lines.append(f"func_L{m}_{place}\tvu0\t{name}{m}\thost\tmap {m}'s copy of {what} "
                             f"(level 39's {l39}; host/trig.c)\t{sig}")
        macros.append(f"MP_MAP_VU0({m}, " + ", ".join(vu0) + ")")
    return entries, lines, macros


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--source", type=Path, default=ROOT / "games/rac3/decomp")
    ap.add_argument("--check", action="store_true", help="fail if a written file would change")
    args = ap.parse_args()
    entries, lines, macros = build(args.source)

    cfg_path = HERE / "hostgen.json"
    cfg = json.loads(cfg_path.read_text())
    base_roots = [r for r in cfg["roots"] if not r.startswith(tuple(f"func_L{m}_" for m in range(40, 56)))]
    roots = base_roots[:]
    inner = []
    for m in MAPS:
        # The entry, and the routine 0x484 past it, which main calls through a pointer: a second
        # entry of the same function, whose jump table's labels lie on both sides of it, so it is
        # an "inner_entries" one, no start that cuts the function in two.
        roots += [f"func_L{m}_{entries[m]:08X}", f"func_L{m}_{entries[m] + SECOND_ENTRY:08X}"]
        inner.append(f"func_L{m}_{entries[m] + SECOND_ENTRY:08X}")
    cfg["roots"] = roots
    cfg["inner_entries"] = inner
    cfg["inner_extents"] = {"places": "config/levels/functions.tsv", "levels": list(MAPS)}
    levels = [39, *MAPS]
    cfg["asm"]["discover_levels"] = levels
    cfg["asm"]["retail"]["levels"] = levels
    cfg["level_copies"] = {"places": "config/levels/functions.tsv", "from": 39, "levels": list(MAPS)}
    own = {k: v for k, v in cfg["asm"]["signatures"].items() if not re.match(r"func_L(4\d|5\d)_", k)}
    # A row the port answers itself (port host) takes its signature from the row, not "asm".
    host_rows = {line.split("\t")[0] for line in lines if line.split("\t")[3] == "host"}
    cfg["asm"]["signatures"] = own
    cfg["asm"]["copy_signatures"] = {k: v for k, v in sorted(map_signatures(
        args.source, {"asm": {"signatures": own}}).items()) if k not in host_rows}
    outputs = {cfg_path: json.dumps(cfg, indent=2) + "\n"}

    lib_path = HERE / "libraries.tsv"
    text = lib_path.read_text()
    if BEGIN in text:
        head, _, rest = text.partition(BEGIN)
        _, _, tail = rest.partition(END + "\n")
    else:
        head, tail = text, ""
    outputs[lib_path] = head + BEGIN + "\n" + "\n".join(lines) + "\n" + END + "\n" + tail

    header = HERE.parent / "host/mp_maps.h"
    outputs[header] = (
        "/* SPDX-License-Identifier: GPL-3.0-or-later\n"
        " * Copyright (c) 2026 the OpenRAC contributors\n *\n"
        " * Written by port/game/rac3-pal/mp/maps.py from rac3-decomp's catalogue: do not edit by hand.\n"
        " * Each multiplayer map's places of the functions mp/maps.py CARRIED lists, in its order. */\n"
        + "/* clang-format off */\n" + "\n".join(macros) + "\n/* clang-format on */\n")

    stale = [p for p, t in outputs.items() if not p.is_file() or p.read_text() != t]
    if args.check:
        for p in stale:
            print(f"maps.py: {p.relative_to(ROOT)} is not what mp/maps.py writes", file=sys.stderr)
        return 1 if stale else 0
    for p in stale:
        p.write_text(outputs[p])
        print(f"wrote {p.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
