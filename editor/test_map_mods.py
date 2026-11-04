"""Named map packs: validated starts, one active map, atomic stack updates and removal."""

import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import overrides
import map_mod
from test_overrides import glb


def manifest(**settings):
    return {"format": overrides.MAP_PACK_FORMAT, "game": "rac1", "version": "pal", "name": "A new arena",
            "map": {"level": 0, "spawn": [20.0, 30.0, 4.0], **settings}}


class MapMods(unittest.TestCase):
    def test_export_round_trip_and_no_overwrite(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            packed, project, out = root / "packed", root / "project", root / "export"
            packed.mkdir()
            (packed / "packed.json").write_text(json.dumps({"version": "pal"}))
            (packed / "level_00").mkdir()
            (packed / "level_01").mkdir()
            (packed / "level_00/core_pal.bin").write_bytes(bytes(2048))
            (packed / "level_01/core_pal.bin").write_bytes(bytes(2048))
            (packed / "level_00/header_pal.bin").write_bytes(bytes(5 * 2048))
            (packed / "level_00/data_pal.bin").write_bytes(bytes(2048))
            (packed / "patches.tsv").write_text("100\t1\tlevel_00/core_pal.bin\n"
                                                  "200\t1\tlevel_01/core_pal.bin\n"
                                                  "10\t5\tlevel_00/header_pal.bin\n")
            (packed / "virtual.tsv").write_text(f"{0x10000000}\t1\tlevel_00/data_pal.bin\t0\t0\t300\t1\n")
            level = project / "levels/level_00"
            level.mkdir(parents=True)
            doc, binary = overrides.glb_parts(glb())
            doc.pop("skins", None)
            for node in doc["nodes"]:
                node.pop("skin", None)
            for mesh in doc["meshes"]:
                for primitive in mesh["primitives"]:
                    primitive["attributes"].pop("JOINTS_0", None)
                    primitive["attributes"].pop("WEIGHTS_0", None)
            source_terrain = overrides.glb_join(doc, binary)
            (level / "terrain.glb").write_bytes(source_terrain)
            manifest = map_mod.export_map(packed, project, out, "My arena", 0, [1, 2, 3], 1.0)
            self.assertEqual(manifest["map"]["spawn"], [1, 2, 3])
            self.assertEqual((out / "level_00/terrain.glb").read_bytes(), source_terrain)
            self.assertIn("100\t1", (out / "patches.tsv").read_text())
            self.assertIn("10\t5\tlevel_00/header_pal.bin", (out / "patches.tsv").read_text())
            self.assertEqual((out / "virtual.tsv").read_text().splitlines()[1],
                             f"{0x10000000}\t1\tlevel_00/data_pal.bin\t0\t0\t300\t1")
            self.assertNotIn("200\t1", (out / "patches.tsv").read_text())
            self.assertFalse((out / "level_01").exists())
            self.assertEqual((level / "terrain.glb").read_bytes(), source_terrain)
            self.assertEqual((packed / "level_00/core_pal.bin").read_bytes(), bytes(2048))
            with self.assertRaisesRegex(overrides.PackError, "already exists"):
                map_mod.export_map(packed, project, out, "Other", 0, [9, 9, 9])
            folder = root / "overrides"
            ident, _, _ = overrides.add(out, folder)
            overrides.stack(folder, root / "mods")
            result = json.loads((root / "mods/map.json").read_text())
            self.assertEqual(result["id"], ident)
            self.assertEqual((result["name"], result["heading"]), ("My arena", 1.0))
            self.assertTrue(result["skip_cutscenes"])
            kept = root / "kept-scenes"
            map_mod.main(["--packed", str(packed), "--project", str(project), "--out", str(kept),
                          "--name", "Keep scenes", "--level", "0", "--spawn", "1", "2", "3",
                          "--keep-movies", "--keep-cutscenes"])
            settings = overrides.map_settings(overrides.read_json(kept / overrides.MANIFEST))
            self.assertFalse(settings["skip_movies"])
            self.assertFalse(settings["skip_cutscenes"])

    def test_normal_pack_has_no_map(self):
        self.assertIsNone(overrides.map_settings({"name": "Textures"}))
        legacy = manifest()
        legacy["format"] = 1
        with self.assertRaisesRegex(overrides.PackError, "format"):
            overrides.read_manifest(json.dumps(legacy).encode(), "legacy")
        del legacy["map"]
        legacy["format"] = 3
        with self.assertRaisesRegex(overrides.PackError, "settings"):
            overrides.read_manifest(json.dumps(legacy).encode(), "missing")

    def test_map_start_defaults_and_validation(self):
        valid = overrides.read_manifest(json.dumps(manifest()).encode(), "arena")
        self.assertEqual(overrides.map_settings(valid),
                         {"level": 0, "spawn": [20.0, 30.0, 4.0], "heading": 0.0,
                          "skip_movies": True, "skip_cutscenes": True})
        for field, values in {"level": [-1, 19, 0.5, True],
                              "spawn": [[], [1, 2], [1, 2, float("nan")], [1, 2, 1e40], [1, 2, 10**400], [True, 2, 3]],
                              "heading": ["0", float("inf")], "skip_movies": [1, "false"],
                              "skip_cutscenes": [0, "true"]}.items():
            for value in values:
                with self.subTest(field=field, value=value), self.assertRaises(overrides.PackError):
                    overrides.read_manifest(json.dumps(manifest(**{field: value})).encode(), "arena")
        wrong_game = manifest()
        wrong_game["game"] = "rac2"
        with self.assertRaises(overrides.PackError):
            overrides.map_settings(wrong_game)

    def test_stack_one_map_disable_and_conflict_preserve_previous_stack(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            packs, out = root / "overrides", root / "mods"
            for ident in ("arena", "another"):
                folder = packs / "packs" / ident
                folder.mkdir(parents=True)
                (folder / overrides.MANIFEST).write_text(json.dumps(manifest()))
            state = {"format": 1, "packs": [{"id": "arena", "enabled": True},
                                             {"id": "another", "enabled": False}]}
            (packs / overrides.STATE).write_text(json.dumps(state))
            overrides.stack(packs, out)
            before = (out / "map.json").read_bytes()
            active = json.loads(before)
            self.assertEqual((active["id"], active["name"], active["version"]),
                             ("arena", "A new arena", "pal"))
            state["packs"][1]["enabled"] = True
            (packs / overrides.STATE).write_text(json.dumps(state))
            with self.assertRaisesRegex(overrides.PackError, "Choose one map"):
                overrides.stack(packs, out)
            self.assertEqual((out / "map.json").read_bytes(), before)
            state["packs"][1]["enabled"] = False
            (packs / overrides.STATE).write_text(json.dumps(state))
            with patch.object(overrides, "Target") as target:
                target.return_value.game = "rac1"
                target.return_value.version = "ntsc"
                with self.assertRaisesRegex(overrides.PackError, "another disc"):
                    overrides.stack(packs, out, levels=root / "other-disc")
            self.assertEqual((out / "map.json").read_bytes(), before)
            for entry in state["packs"]:
                entry["enabled"] = False
            (packs / overrides.STATE).write_text(json.dumps(state))
            overrides.stack(packs, out)
            self.assertFalse((out / "map.json").exists())


if __name__ == "__main__":
    unittest.main()
