# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""run.py's joining of parts: each texture once, with only the blobs it needs, and the textures no
part placed on the disc as missing."""

import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import run  # noqa: E402


def part(textures, blobs, missing=None, game="rac1-ntsc", converter=1):
    return {"format": 1, "game": game, "converter": converter, "blobs": blobs, "textures": textures,
            "missing": missing or {"font": [], "interface": [], "effects": []}}


def pool(name, category, *blobs):
    return {"name": name, "category": category, "kind": "pool", "tex0": "0", "texa": "0",
            "uploads": [{"dbp": 0, "dbw": 1, "dpsm": 0, "x": 0, "y": 0, "w": 1, "h": 1, "blob": b} for b in blobs]}


class JoinTests(unittest.TestCase):
    def test_each_texture_once_with_its_blobs(self):
        a = part([pool("01", "font", "a1")], {"a1": {"size": 64, "at": 2048}, "zz": {"size": 32, "at": 0}})
        b = part([pool("01", "font", "a1"), pool("02", "interface", "b1")],
                 {"a1": {"size": 64, "at": 2048}, "b1": {"size": 64, "wad": 4096, "at": 16}})
        doc = run.join([a, b], "rac1-ntsc")
        self.assertEqual([t["name"] for t in doc["textures"]], ["01", "02"])
        self.assertEqual(sorted(doc["blobs"]), ["a1", "b1"])
        self.assertEqual(doc["counts"], {"font": 1, "interface": 1, "effects": 0})

    def test_missing_only_when_no_part_placed_it(self):
        a = part([], {}, {"font": ["01"], "interface": ["03"], "effects": []})
        b = part([pool("01", "font", "a1")], {"a1": {"size": 64, "at": 0}})
        doc = run.join([a, b], "rac1-ntsc")
        self.assertEqual(doc["missing"], {"font": [], "interface": ["03"], "effects": []})

    def test_texture_without_its_blob_is_left_out(self):
        doc = run.join([part([pool("01", "font", "a1")], {})], "rac1-ntsc")
        self.assertEqual(doc["textures"], [])

    def test_other_games_and_converters_are_left_out(self):
        a = part([pool("01", "font", "a1")], {"a1": {"size": 64, "at": 0}})
        b = part([pool("02", "font", "a1")], {"a1": {"size": 64, "at": 0}}, converter=2)
        c = part([pool("03", "font", "a1")], {"a1": {"size": 64, "at": 0}}, game="rac1-pal")
        doc = run.join([a, b, c], "rac1-ntsc")
        self.assertEqual([t["name"] for t in doc["textures"]], ["01"])
        with self.assertRaises(ValueError):
            run.join([c], "rac1-ntsc")

    def test_cut_short_part_is_not_read(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "part.json"
            path.write_text(json.dumps(part([], {}))[:-5])
            self.assertIsNone(run.read_part(path))
            path.write_text(json.dumps(part([], {})))
            self.assertIsNotNone(run.read_part(path))

    def test_join_command_writes_whole_file(self):
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp) / "p.json"
            p.write_text(json.dumps(part([pool("01", "effects", "a1")], {"a1": {"size": 64, "at": 0}})))
            out = Path(tmp) / "run.json"
            self.assertEqual(run.main(["join", str(out), str(p), "--official"]), 0)
            doc = json.loads(out.read_text())
            self.assertTrue(doc["official"])
            self.assertFalse((Path(tmp) / "run.json.partial").exists())


if __name__ == "__main__":
    unittest.main()
