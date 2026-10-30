# SPDX-License-Identifier: GPL-3.0-or-later
"""Automatic water preparation uses small synthetic overlay tables, never captured assets."""
import json
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch

import bake_water
import prepare_water


def overlay():
    base = 0x1cad00
    tables = bytearray(0x203000-base)
    struct.pack_into('<3f', tables, 0x1e34c0-base, 0, 0, 1)
    struct.pack_into('<I', tables, 0x1fa590-base, 1)
    vertices = bytearray()
    for address, count in ((0x1e2fc0, 8), (0x1e3380, 2), (0x1fbc80, 4), (0x202d80, 3)):
        for layer in range(count):
            struct.pack_into('<I', tables, address+layer*0x60-base, 0x300000+len(vertices))
            struct.pack_into('<I', tables, address+layer*0x60+12-base, 3)
            for point in ((0, 0, 2, 0), (1, 0, 2, 0), (0, 1, 2, 0)):
                vertices.extend(struct.pack('<4f', *point))
    return (struct.pack('<4I', base, len(tables), 0, base)+tables
            +struct.pack('<4I', 0x300000, len(vertices), 0, base)+vertices)


class Preparation(unittest.TestCase):
    def test_native_baker_runs_before_publication_and_cache_tracks_its_profile(self):
        profile = {'version': 1, 'algorithm': 'fixture-v1'}
        def native(memory, collision, output, baker, **kwargs):
            self.assertEqual(baker,Path('native-baker'))
            self.assertNotEqual(output,self.level/'water.bin')
            output.write_bytes(b'water')
            output.with_suffix('.mesh').write_bytes(b'mesh')
        with patch.object(prepare_water.subprocess,'check_output',side_effect=lambda *a,**kw:json.dumps(profile)), \
             patch.object(bake_water,'bake_native',side_effect=native) as run:
            prepare_water.prepare(self.level,self.manifest,baker=Path('native-baker'))
            prepare_water.prepare(self.level,self.manifest,baker=Path('native-baker'))
            self.assertEqual(run.call_count,1)
            profile['algorithm']='fixture-v2'
            prepare_water.prepare(self.level,self.manifest,baker=Path('native-baker'))
            self.assertEqual(run.call_count,2)
            (self.level/'water.mesh').write_bytes(b'corrupt')
            prepare_water.prepare(self.level,self.manifest,baker=Path('native-baker'))
            self.assertEqual(run.call_count,3)
        self.assertEqual((self.level/'water.bin').read_bytes(), b'water')

    def test_failed_native_mesh_keeps_both_published_files(self):
        """A native failure after writing fields must not publish an incomplete pair."""
        (self.level/'water.bin').write_bytes(b'previous water')
        (self.level/'water.mesh').write_bytes(b'previous mesh')

        def incomplete(memory, collision, output, baker, **kwargs):
            output.write_bytes(b'unpublished water')
            raise ValueError('cannot write rest mesh')

        with patch.object(prepare_water.subprocess, 'check_output', return_value='{}'), \
             patch.object(bake_water, 'bake_native', side_effect=incomplete):
            with self.assertRaisesRegex(ValueError, 'rest mesh'):
                prepare_water.prepare(self.level, self.manifest, baker=Path('native-baker'))
        self.assertEqual((self.level/'water.bin').read_bytes(), b'previous water')
        self.assertEqual((self.level/'water.mesh').read_bytes(), b'previous mesh')
        self.assertFalse((self.level/'water-bake.json').exists())

    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.level = Path(temporary.name)
        (self.level/'bake-inputs').mkdir()
        (self.level/'bake-inputs/overlay.bin').write_bytes(overlay())
        (self.level/'collision.glb').write_bytes(b'collision')
        self.manifest = {'game': 'rac1', 'version': 'ntsc', 'level': 1,
                         'bake_inputs': {'water': 'profile-v1'}}

    def test_offline_initialization_preserves_all_surfaces_and_strip_rest_height(self):
        memory = prepare_water.overlay_memory(overlay())
        surfaces = bake_water.read_surfaces(memory)
        self.assertEqual({kind for kind, _, _ in surfaces}, set(range(1, 8)))
        strip = next(triangles for kind, triangles, _ in surfaces if kind == 4)
        self.assertAlmostEqual(strip[0][0][2], 1.55, places=6)
        self.assertEqual(struct.unpack_from('<2f', memory, 0x1cad10), (1, 1))

    def test_invalid_overlay_fails(self):
        for data in (b'truncated', struct.pack('<4I', 0x1cad00, 0, 0, 0x1cad00)):
            with self.assertRaises(ValueError):
                prepare_water.overlay_memory(data)

    def test_real_bake_writes_matching_water_and_mesh_fingerprints(self):
        with patch.object(bake_water, 'collision_triangles', return_value=([], [])):
            prepare_water.prepare(self.level, self.manifest)
        data = (self.level/'water.bin').read_bytes()
        mesh = (self.level/'water.mesh').read_bytes()
        self.assertEqual(data[:8], b'ORWATER1')
        self.assertEqual(mesh[:8], b'ORWMESH2')
        self.assertEqual(data[20:52], mesh[16:48])
        self.assertEqual(len(data[20:52]), 32)

    def test_old_export_recovers_overlay_from_disc(self):
        from types import SimpleNamespace
        raw = overlay()
        content = struct.pack('<2I', 8, len(raw))+raw
        disc = SimpleNamespace(version=SimpleNamespace(key='ntsc'),
                               survey=lambda: {'levels': [None, {'ranges': {'data': {'lba': 4, 'sectors': 1}}}]},
                               sectors=lambda *_: content)
        (self.level/'bake-inputs/overlay.bin').unlink()
        with patch('disc.Disc') as reader:
            reader.return_value.__enter__.return_value = disc
            self.assertEqual(prepare_water.source_overlay(self.level, Path('disc.iso')).read_bytes(), raw)
        self.assertEqual(prepare_water.source_overlay(self.level, None).read_bytes(), raw)

    def test_fresh_export_bakes_once_and_missing_or_changed_files_rebuild(self):
        def bake(memory, collision, output, boundaries):
            bake_water.read_surfaces(memory)
            output.write_bytes(b'water')
            output.with_suffix('.mesh').write_bytes(b'mesh')
        with patch.object(bake_water, 'bake', side_effect=bake) as run:
            prepare_water.prepare(self.level, self.manifest)
            self.assertEqual(run.call_count, 1)
            prepare_water.prepare(self.level, self.manifest)
            self.assertEqual(run.call_count, 1)
            (self.level/'water.mesh').unlink()
            prepare_water.prepare(self.level, self.manifest)
            self.assertEqual(run.call_count, 2)
            (self.level/'collision.glb').write_bytes(b'new collision')
            prepare_water.prepare(self.level, self.manifest)
            self.assertEqual(run.call_count, 3)
            (self.level/'water-boundaries.json').write_text('{"open_edges":[]}')
            prepare_water.prepare(self.level, self.manifest)
            self.assertEqual(run.call_count, 4)
            (self.level/'water.bin').write_bytes(b'corrupt')
            prepare_water.prepare(self.level, self.manifest)
            self.assertEqual(run.call_count, 5)
            stamp = self.level/'water-bake.json'
            previous = json.loads(stamp.read_text())
            stamp.write_text(json.dumps({**previous, 'version': 2}))
            prepare_water.prepare(self.level, self.manifest)
            self.assertEqual(run.call_count, 6)
            stamp.write_text('[]')
            prepare_water.prepare(self.level, self.manifest)
            self.assertEqual(run.call_count, 7)

    def test_failed_bake_preserves_previous_assets(self):
        (self.level/'water.bin').write_bytes(b'previous water')
        (self.level/'water.mesh').write_bytes(b'previous mesh')
        with patch.object(bake_water, 'bake', side_effect=ValueError('bad geometry')):
            with self.assertRaisesRegex(ValueError, 'bad geometry'):
                prepare_water.prepare(self.level, self.manifest)
        self.assertEqual((self.level/'water.bin').read_bytes(), b'previous water')
        self.assertEqual((self.level/'water.mesh').read_bytes(), b'previous mesh')
        self.assertFalse((self.level/'water-bake.json').exists())

    def test_unsupported_levels_are_untouched_and_missing_inputs_are_actionable(self):
        with patch.object(bake_water, 'bake') as run:
            for fields in ({'version': 'pal'}, {'level': 0}, {'game': 'rac2'}):
                prepare_water.prepare(self.level, {**self.manifest, **fields})
            run.assert_not_called()
        (self.level/'bake-inputs/overlay.bin').unlink()
        with self.assertRaisesRegex(ValueError, 're-extract.*--disc'):
            prepare_water.prepare(self.level, self.manifest)


if __name__ == '__main__':
    unittest.main()
