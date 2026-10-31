import json
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

from enhanced.bake_inputs import write_bake_inputs


class BakeInputs(unittest.TestCase):
    def test_retains_source_without_analysis_or_proxies(self):
        source = {"overlay": b"source overlay", "core_index": b"source index", "core_data": b"source core"}
        level = SimpleNamespace(game="rac1", version="ntsc", id=9, stored=source, gameplay=b"gameplay")
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            metadata = write_bake_inputs(level, root)
            self.assertEqual(metadata, {"gameplay": "rac1-v1", "liquid": "rac1-ntsc-gaspar-v1"})
            self.assertEqual(sorted(p.name for p in root.iterdir()), ["bake-inputs"])
            for name, data in source.items():
                self.assertEqual((root/"bake-inputs"/(name+".bin")).read_bytes(), data)

    def test_pal_names_its_own_liquid_source(self):
        source = {"overlay": b"o", "core_index": b"i", "core_data": b"c"}
        level = SimpleNamespace(game="rac1", version="pal", id=9, stored=source, gameplay=b"gameplay")
        with tempfile.TemporaryDirectory() as temporary:
            metadata = write_bake_inputs(level, Path(temporary))
            self.assertEqual(metadata, {"gameplay": "rac1-v1", "liquid": "rac1-pal-gaspar-v1"})

    def test_normal_port_export_registers_raw_sources(self):
        from test_port import synthetic_level
        from port import PortLevelWriter
        level = synthetic_level()
        level.id = 9
        level.version = "ntsc"
        level.stored = {name: name.encode() for name in ("overlay", "core_index", "core_data")}
        with tempfile.TemporaryDirectory() as temporary:
            writer = PortLevelWriter(Path(temporary), level)
            writer.dir.mkdir(parents=True)
            writer.finish()
            manifest = json.loads((writer.dir/"manifest.json").read_text())
            self.assertEqual(manifest["bake_inputs"], {"gameplay":"rac1-v1", "liquid":"rac1-ntsc-gaspar-v1"})
            self.assertFalse((writer.dir/"probe-surfaces").exists())
            self.assertTrue((writer.dir/"bake-inputs/overlay.bin").is_file())

    def test_other_levels_and_versions_are_unchanged(self):
        for game, version, level_id in [("rac2", "ntsc", 9), ("rac1", "unknown", 3)]:
            level = SimpleNamespace(game=game, version=version, id=level_id)
            with tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                self.assertIsNone(write_bake_inputs(level, root))
                self.assertEqual(list(root.iterdir()), [])

    def test_missing_source_is_an_error(self):
        level = SimpleNamespace(game="rac1", version="ntsc", id=9, stored={}, gameplay=b"gameplay")
        with tempfile.TemporaryDirectory() as temporary:
            with self.assertRaises(ValueError):
                write_bake_inputs(level, Path(temporary))

    def test_gameplay_retained_for_all_rac1_levels(self):
        for version in ('pal', 'ntsc'):
            level = SimpleNamespace(game='rac1', version=version, id=2, gameplay=b'authored gameplay')
            with tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                self.assertEqual(write_bake_inputs(level, root), {'gameplay': 'rac1-v1'})
                self.assertEqual((root/'bake-inputs/gameplay.bin').read_bytes(), level.gameplay)
                self.assertEqual(len(list((root/'bake-inputs').iterdir())), 1)

    def test_novalis_export_retains_water_overlay(self):
        from test_port import synthetic_level
        for version in ('ntsc', 'pal'):
            from port import PortLevelWriter
            level = synthetic_level()
            level.id, level.version = 1, version
            level.stored = {'overlay': b'authored water tables'}
            with tempfile.TemporaryDirectory() as temporary:
                writer = PortLevelWriter(Path(temporary), level)
                writer.dir.mkdir(parents=True)
                writer.finish()
                manifest = json.loads((writer.dir/'manifest.json').read_text())
                self.assertEqual(manifest['bake_inputs']['water'], 'profile-v1')
                self.assertEqual((writer.dir/'bake-inputs/overlay.bin').read_bytes(), level.stored['overlay'])


    def test_water_profile_retains_overlay_for_any_configured_level(self):
        from enhanced import profiles
        level = SimpleNamespace(game='rac1', version='pal', id=12,
                                gameplay=b'gameplay', stored={'overlay':b'configured tables'})
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            profile = root/'profiles/rac1-pal/level_12/probe-bake.json'
            profile.parent.mkdir(parents=True)
            profile.write_text('{"version":1,"water_sources":{"version":1}}')
            export = root/'export'
            export.mkdir()
            with patch.object(profiles,'PROFILE_ROOT',root/'profiles'):
                metadata = write_bake_inputs(level,export)
                profiles.write_bake_profile(level,export)
            self.assertEqual(metadata['water'],'profile-v1')
            self.assertEqual((export/'bake-inputs/overlay.bin').read_bytes(),level.stored['overlay'])
            self.assertEqual((export/'probe-bake.json').read_bytes(),profile.read_bytes())
