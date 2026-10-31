# SPDX-License-Identifier: GPL-3.0-or-later
"""Persistent native interior preparation selection and legacy gameplay retention."""
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import automatic_interiors as automatic
from prepare_interiors import prepare_export
from test_seed_regions import gameplay


class AutomaticInteriors(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.level, self.output = self.root/'level', self.root/'output'
        self.level.mkdir()
        self.output.mkdir()
        self.manifest = dict(game='rac1', version='ntsc', level=0)

    def test_exported_gameplay_is_passed_to_the_persistent_worker(self):
        inputs = self.level/'bake-inputs'
        inputs.mkdir()
        path = inputs/'gameplay.bin'
        path.write_bytes(b'gameplay fixture')
        self.assertEqual(automatic.native_options(self.level, self.output, self.manifest, path.read_bytes()),
                         ['--prepare-interiors', str(path)])
        self.assertEqual(list(self.output.iterdir()), [])

    def test_legacy_gameplay_is_retained_without_an_inference_subprocess(self):
        options = automatic.native_options(self.level, self.output, self.manifest, b'owned disc gameplay')
        self.assertEqual(options[0], '--prepare-interiors')
        self.assertEqual(Path(options[1]).read_bytes(), b'owned disc gameplay')
        self.assertFalse(list(self.output.glob('.interior-gameplay-*')))

    def test_changed_gameplay_is_rejected_before_native_preparation(self):
        inputs = self.level/'bake-inputs'
        inputs.mkdir()
        (inputs/'gameplay.bin').write_bytes(b'new gameplay')
        with self.assertRaisesRegex(ValueError, 'source changed'):
            automatic.native_options(self.level, self.output, self.manifest, b'previous gameplay')

    def test_non_gameplay_scenes_skip_preparation(self):
        for manifest, data in ((self.manifest, None), ({**self.manifest, 'level': 99}, b'gameplay')):
            self.assertEqual(automatic.native_options(self.level, self.output, manifest, data), [])

    def test_opt_out_removes_only_generated_sources(self):
        sidecar = self.level/'probe-interiors.json'
        sidecar.write_text('{"generator":"openrac-interiors-v1"}')
        automatic.remove_generated(self.level)
        self.assertFalse(sidecar.exists())
        sidecar.write_text('{"version":1,"samples":[]}')
        automatic.remove_generated(self.level)
        self.assertEqual(json.loads(sidecar.read_text()), {'version': 1, 'samples': []})

    def test_exported_collision_keeps_water_non_walkable_and_authored_transitions(self):
        from collision import colour
        water = [channel ** 2.2 for channel in colour(0)]+[1]

        class Document:
            def list(self, key):
                return {'nodes': [{'mesh': 0}], 'meshes': [{'primitives': [
                    {'attributes': {'POSITION': 0, 'COLOR_0': 1}}]}]}[key]

            def node_world(self, index):
                return [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]

            def accessor(self, index):
                return ([[0, 0, 0], [1, 0, 0], [0, 1, 0]]*2 if index == 0
                        else [water]*3+[[.5, .5, .5, 1]]*3)

        (self.level/'collision.glb').write_bytes(b'fixture')
        with patch('gltf_read.read', return_value=Document()):
            result = prepare_export(self.level, self.manifest, gameplay([1]))
        self.assertEqual([t[9] for t in result['triangles']], [0, 1])
        self.assertEqual(len(result['transitions']), 1)
