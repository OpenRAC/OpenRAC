"""Typed edits in the installed editor, with no decompilation checkout available."""
import json
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch

import gamedata as gd
from pack import pack_level
from test_gamedata import build, scene_text, edit, LAYOUT


class PackagedPvars(unittest.TestCase):
    def test_generated_layouts_preserve_typed_edits_without_source_headers(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            path = root / 'pvar_layouts.json'
            with patch.object(gd, '_layouts', LAYOUT):
                gd.export_layouts(path)
                original = build()
                scene = scene_text(original)
            with patch.object(gd, '_layouts', None), patch.object(gd, 'DECOMPILATION', root / 'no-checkout'), \
                    patch.object(gd, 'PACKAGED_LAYOUTS', path):
                text = edit(scene, 'BoltCrate_0001', 'metadata/rc1_pv_speed', '10')
                packed, changes, problems = pack_level(0, original, text)
                self.assertEqual(problems, [])
                self.assertEqual(changes, ['BoltCrate_0001: pvars'])
                pvars = gd.Pvars(packed)
                self.assertEqual(struct.unpack_from('<f', pvars.block(1), 0x10)[0], 10.0)
                baseline, changes, problems = pack_level(0, original, scene)
                self.assertEqual((baseline, changes, problems), (original, [], []))

    def test_corrupt_schema_is_rejected_on_every_attempt(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            path = root / 'pvar_layouts.json'
            for schema in ({'format': 9, 'classes': {}}, [],
                           {'format': 1, 'classes': {'726': [['speed', -1, 'f32', 0]]}},
                           {'format': 1, 'classes': {'726': [['speed', 0, [], 0]]}}):
                path.write_text(json.dumps(schema))
                with patch.object(gd, '_layouts', None), patch.object(gd, 'DECOMPILATION', root / 'absent'), \
                        patch.object(gd, 'PACKAGED_LAYOUTS', path):
                    for _ in range(2):
                        with self.assertRaises(gd.FormatError):
                            gd.class_layouts()

    def test_missing_optional_definitions_keep_raw_word_support(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            with patch.object(gd, '_layouts', None), patch.object(gd, 'DECOMPILATION', root), \
                    patch.object(gd, 'PACKAGED_LAYOUTS', root / 'missing.json'):
                self.assertEqual(gd.class_layouts(), {})


if __name__ == '__main__':
    unittest.main()
