# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""run.py's joining of parts: each texture once, with only the blobs it needs, and the textures no
part placed on the disc as missing."""

import json
import sys
import tempfile
import unittest
import unittest.mock
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

    def test_broken_textures_are_left_out(self):
        good = pool("01", "font", "a1")
        doc = run.join([part([good, {"name": "02", "category": "hud", "kind": "pool", "uploads": []},
                              {"name": "03", "category": "font", "kind": "pool", "uploads": []},
                              "not a texture"], {"a1": {"size": 64, "at": 0}})], "rac1-ntsc")
        self.assertEqual([t["name"] for t in doc["textures"]], ["01"])

    def test_env_must_be_key_value(self):
        self.assertEqual(run.env_pair("A=b=c"), ("A", "b=c"))
        for bad in ("A", "=b"):
            with self.assertRaises(Exception):
                run.env_pair(bad)

    def test_no_route_to_play_is_an_error(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe = Path(tmp) / "game"
            exe.write_text("")
            out = Path(tmp) / "run.json"
            lines = []
            with unittest.mock.patch.object(run, "say", lambda e, **f: lines.append((e, f))):
                code = run.main(["run", "--game", "rac1-ntsc", "--out", str(out), "--exe", str(exe),
                                 "--data", tmp, "--levels", tmp, "--only", "no-such-route"])
            self.assertEqual(code, 1)
            self.assertEqual(lines[-1][0], "error")
            self.assertFalse(out.exists())

    def test_card_route_plays_new_game_on_an_empty_card(self):
        route = run.base_route("rac1-ntsc")
        self.assertEqual((route.card, route.start, route.key), ("empty", "title", "card"))
        self.assertTrue(route.press().startswith("100:"))  # the first press is at the title
        self.assertIsNone(run.base_route("rac2-ntsc"))  # no card can be made there yet



class KindsTests(unittest.TestCase):
    """A run of some kinds plays only their routes; a later one adds to it."""

    TABLE = {"rac1-ntsc": {"font": ["title-new"], "interface": ["00-long", "title-load-0"]}}

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        root = Path(self.tmp.name)
        self.exe, self.out = root / "game", root / "runs" / "rac1-ntsc.json"
        self.exe.write_text("")
        self.played, self.converter = [], 1

    def tearDown(self):
        self.tmp.cleanup()

    def fake_level(self, game, route, exe, data, levels, out, env, settings, extra, cards=None):
        self.played.append(route.key)
        out.mkdir(parents=True, exist_ok=True)
        category = "font" if route.key == "title-new" else "interface" if route.key in ("00-long", "title-load-0") \
            else "effects"
        doc = part([pool(route.key, category, "b" + route.key)], {"b" + route.key: {"size": 64, "at": 0}},
                   converter=self.converter)
        (out / "part.json").write_text(json.dumps(doc))
        return {"status": "ok"}

    def make(self, *extra):
        self.played = []
        with unittest.mock.patch.object(run, "KIND_ROUTES", self.TABLE), \
                unittest.mock.patch.object(run.runner, "run_level", self.fake_level), \
                unittest.mock.patch.object(run, "make_card", lambda *a: Path(self.tmp.name)), \
                unittest.mock.patch.object(run, "say", lambda e, **f: None):
            code = run.main(["run", "--game", "rac1-ntsc", "--out", str(self.out), "--exe", str(self.exe),
                             "--data", self.tmp.name, "--levels", self.tmp.name, "--jobs", "1", *extra])
        self.assertEqual(code, 0)
        return json.loads(self.out.read_text())

    def test_a_kind_then_another_adds_to_the_run(self):
        doc = self.make("--kinds", "font")
        self.assertEqual(self.played, ["title-new"])
        self.assertEqual({k: v["complete"] for k, v in doc["kinds"].items()},
                         {"font": True, "interface": False, "effects": False})
        doc = self.make("--kinds", "interface")
        self.assertEqual(sorted(self.played), ["00-long", "title-load-0"])
        self.assertEqual(sorted(t["name"] for t in doc["textures"]), ["00-long", "title-load-0", "title-new"])
        self.assertTrue(doc["kinds"]["font"]["complete"] and doc["kinds"]["interface"]["complete"])
        self.assertTrue(doc["clean"])

    def test_a_kind_made_already_plays_one_route_to_check_the_port(self):
        self.make("--kinds", "font,interface")
        self.make("--kinds", "interface")
        self.assertEqual(self.played, ["00-long"])  # the short one without a card

    def test_an_earlier_run_of_another_conversion_is_made_again(self):
        self.make("--kinds", "interface")
        self.converter = 2
        doc = self.make("--kinds", "interface")
        self.assertEqual(sorted(self.played), ["00-long", "title-load-0"])
        self.assertEqual(doc["converter"], 2)

    def test_fresh_starts_over(self):
        self.make("--kinds", "font")
        doc = self.make("--kinds", "interface", "--fresh")
        self.assertEqual([t["name"] for t in doc["textures"]], sorted(["00-long", "title-load-0"]))
        self.assertFalse(doc["kinds"]["font"]["complete"])

    def test_kinds_must_be_known(self):
        with self.assertRaises(Exception):
            run.kinds_arg("font,hud")

    def test_every_listed_route_is_one_of_the_set(self):
        for game, table in run.KIND_ROUTES.items():
            keys = {r.key for r in run.routes_mod.set_routes(run.ROUTE_SETS[game], game, run.routes_mod.load())}
            for kind, routes in table.items():
                self.assertIn(kind, run.CATEGORIES)
                self.assertLessEqual(set(routes), keys, f"{game} {kind}")


if __name__ == "__main__":
    unittest.main()
