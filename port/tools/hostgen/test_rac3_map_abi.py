#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Checks the generated map wrapper contract without reading disc data.

Run with python -m unittest discover -s port/tools/hostgen -p test_rac3_map_abi.py.
The layout's four-int host ABI must survive alongside the collision wrapper when tables merge.
"""
import importlib.util
import json
from pathlib import Path
import re
import unittest

GAME = Path(__file__).resolve().parents[2] / "game"
SPEC = importlib.util.spec_from_file_location("rac3_maps", GAME / "rac3-pal/mp/maps.py")
MAPS = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MAPS)


class MapAbi(unittest.TestCase):
    def test_layout_and_collision_keep_their_own_macro_slots(self):
        """Both PAL and carried NTSC macro rows have the generator's arity and library order."""
        for version in ("rac3-pal", "rac3-ntsc"):
            with self.subTest(version=version):
                root = GAME / version
                library = {
                    cols[0]: cols for line in (root / "mp/libraries.tsv").read_text().splitlines()
                    if line and not line.startswith("#") and len(cols := line.split("\t")) >= 6
                }
                rows = re.findall(r"^MP_MAP\((\d+), (.*)\)$",
                                  (root / "host/mp_maps.h").read_text(), re.M)
                expected = range(40, 54 if version == "rac3-pal" else 56)
                self.assertEqual({int(level) for level, _ in rows}, set(expected))
                for level, addresses in rows:
                    places = addresses.split(", ")
                    self.assertEqual(len(places), len(MAPS.CARRIED))
                    for place, (_, _, name, mode, _, signature) in zip(places, MAPS.CARRIED):
                        row = library[f"func_L{level}_{place}"]
                        self.assertEqual(row[2:4], [name + level, mode])
                        self.assertEqual(row[5], signature)

    def test_layout_wrappers_and_translated_callees_take_four_ints(self):
        """The library row and translated original agree on the draft's four-int layout ABI."""
        signature = "void (int, int, int, int)"
        carried = next(row for row in MAPS.CARRIED if row[2] == "rac3ViewLayoutMap")
        self.assertEqual(carried[3], "wrap")
        self.assertEqual(carried[5], signature)
        for version in ("rac3-pal", "rac3-ntsc"):
            root = GAME / version
            cfg = json.loads((root / "mp/hostgen.json").read_text())["asm"]
            signatures = cfg["signatures"] | cfg.get("copy_signatures", {})
            for line in (root / "mp/libraries.tsv").read_text().splitlines():
                cols = line.split("\t")
                if len(cols) < 6 or not re.fullmatch(r"rac3ViewLayoutMap\d+", cols[2]):
                    continue
                with self.subTest(version=version, function=cols[0]):
                    self.assertEqual(cols[5], signature)
                    self.assertEqual(signatures[cols[0]], signature)


if __name__ == "__main__":
    unittest.main()
