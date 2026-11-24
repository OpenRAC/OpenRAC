#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""The multiplayer maps' 2D and HUD rows (rac3-pal/mp/maps.py CARRIED_UI, CARRIED_HUD) as the host C
expands them, on PAL's tables and NTSC-U's carried ones, without reading disc data.

    python -m unittest discover -s port/tools/hostgen -p test_rac3_map_ui_rows.py

Each MP_MAP_UI and MP_MAP_HUD line has one place per entry, and each macro the host C defines over
them names that many; every wrap row of a map has a signature, its own or its copy's ("asm"
"copy_signatures"), so a build without the disc declares it; the rows that carry their own keep it.
"""
import importlib.util
import json
from pathlib import Path
import re
import sys
import unittest
from unittest import mock

GAME = Path(__file__).resolve().parents[2] / "game"
SPEC = importlib.util.spec_from_file_location("rac3_maps", GAME / "rac3-pal/mp/maps.py")
MAPS = importlib.util.module_from_spec(SPEC)
# Match the script's CLI import context: maps.py imports its sibling fixes.py.
with mock.patch("sys.path", [str(GAME / "rac3-pal/mp"), *sys.path]):
    SPEC.loader.exec_module(MAPS)
HOST = GAME / "rac3-pal/host"


def macro_slots(path: Path, name: str) -> int:
    """How many places after the map's number the host C's #define of `name` takes."""
    m = re.search(rf"#define {name}\(([^)]*)\)", path.read_text().replace("\\\n", " "))
    assert m, f"{path.name} defines no {name}"
    params = [p.strip() for p in m.group(1).split(",")]
    return len(params) - 1


class MapUiRows(unittest.TestCase):
    def test_host_macros_take_every_entry(self):
        """ui2d.c's MP_MAP_UI, hud.c's and textures.c's MP_MAP_HUD name each entry's place."""
        self.assertEqual(macro_slots(HOST / "ui2d.c", "MP_MAP_UI"), len(MAPS.CARRIED_UI))
        self.assertEqual(macro_slots(HOST / "hud.c", "MP_MAP_HUD"), len(MAPS.CARRIED_HUD))
        self.assertEqual(macro_slots(HOST / "textures.c", "MP_MAP_HUD"), len(MAPS.CARRIED_HUD))

    def test_lines_rows_and_signatures(self):
        """Both versions' lines have the entries' arity; each wrap place has its row and a signature."""
        for version, maps in (("rac3-pal", range(40, 54)), ("rac3-ntsc", range(40, 56))):
            root = GAME / version
            cfg = json.loads((root / "mp/hostgen.json").read_text())["asm"]
            signatures = cfg["signatures"] | cfg.get("copy_signatures", {})
            library = {
                cols[0]: cols for line in (root / "mp/libraries.tsv").read_text().splitlines()
                if line and not line.startswith("#") and len(cols := line.split("\t")) >= 5
            }
            header = (root / "host/mp_maps.h").read_text()
            for macro, carried in (("MP_MAP_UI", MAPS.CARRIED_UI), ("MP_MAP_HUD", MAPS.CARRIED_HUD)):
                lines = re.findall(rf"^{macro}\((\d+), (.*)\)$", header, re.M)
                with self.subTest(version=version, macro=macro):
                    self.assertEqual({int(level) for level, _ in lines}, set(maps))
                for level, places in lines:
                    places = places.split(", ")
                    self.assertEqual(len(places), len(carried), f"{version} {macro}({level})")
                    for place, entry in zip(places, carried):
                        if entry[3] == "ref":
                            continue
                        name = f"func_L{level}_{place}"
                        with self.subTest(version=version, function=name):
                            row = library[name]
                            self.assertEqual(row[3], entry[3])
                            self.assertTrue(row[2].startswith(entry[2]))
                            own = row[5] if len(row) > 5 else ""
                            self.assertTrue(own or name in signatures, "no signature")
                            if len(entry) > 5:
                                self.assertEqual(own, entry[5])

    def test_scissor_and_send_image_signatures(self):
        """The rows this level 39 has no "asm" signature for carry the host C's."""
        by_name = {entry[2]: entry for entry in (*MAPS.CARRIED_UI, *MAPS.CARRIED_HUD)}
        self.assertEqual(by_name["SetScissorMap"][5], "int (int, int, int, int)")
        self.assertEqual(by_name["SendImageMap"][5], "int (int, int, int, int, int, int)")
        self.assertEqual(by_name["SendImageMap"][6], "host/textures.c")


if __name__ == "__main__":
    unittest.main()
