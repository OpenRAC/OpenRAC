# SPDX-License-Identifier: GPL-3.0-or-later
"""Default interior preparation, ownership, cache invalidation and exported collision."""
import json
from pathlib import Path
import subprocess
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
        self.baker = self.root/'openrac-probe-bake.exe'
        automatic.inference_tool(self.baker).write_bytes(b'inference fixture')
        self.manifest = dict(game='rac1', version='ntsc', level=0)
        self.inputs = dict(source_sha256='a'*64)
        self.report = dict(format=1, game='rac1-ntsc', level=0, source_sha256='a'*64,
                           transitions=[], seeds=[], regions=[], cells=[])
        self.sidecar = self.level/'probe-interiors.json'

    def generate(self, command, **kwargs):
        Path(command[2]).write_text(json.dumps(self.report))

    def prepare(self):
        automatic.prepare(self.level, self.output, self.baker, self.manifest, b'gameplay')

    def test_default_generation_reuses_unchanged_sources_and_refreshes_changed_inputs(self):
        with patch.object(automatic, 'prepare_export', return_value=self.inputs), \
             patch.object(automatic.subprocess, 'run', side_effect=self.generate) as run:
            self.prepare()
            self.assertEqual(json.loads(self.sidecar.read_text())['generator'], 'openrac-interiors-v1')
            self.prepare()
            self.assertEqual(run.call_count, 1)
            self.output = self.root/'second'
            self.output.mkdir()
            self.inputs['source_sha256'] = 'b'*64
            self.report['source_sha256'] = 'b'*64
            self.prepare()
            self.assertEqual(run.call_count, 2)
            self.assertEqual(json.loads(self.sidecar.read_text())['source_sha256'], 'b'*64)

    def test_missing_report_and_stale_report_are_regenerated_safely(self):
        with patch.object(automatic, 'prepare_export', return_value=self.inputs), \
             patch.object(automatic.subprocess, 'run', side_effect=self.generate) as run:
            self.prepare()
            (self.output/'interior-inference.json').unlink()
            self.prepare()
            self.assertEqual(run.call_count, 2)
            self.inputs['source_sha256'] = 'c'*64
            self.prepare()  # Existing report/log must not block regeneration.
            self.assertEqual(run.call_count, 3)
        self.assertFalse(list(self.output.glob('.interior-inference-*')))

    def test_explicit_sources_are_preserved(self):
        self.sidecar.write_text('{"version":1,"samples":[]}')
        with patch.object(automatic, 'prepare_export') as read:
            self.prepare()
        read.assert_not_called()
        self.assertEqual(self.sidecar.read_text(), '{"version":1,"samples":[]}')

    def test_opt_out_removes_only_generated_sources(self):
        self.sidecar.write_text('{"generator":"openrac-interiors-v1"}')
        automatic.remove_generated(self.level)
        self.assertFalse(self.sidecar.exists())
        self.sidecar.write_text('{"version":1,"samples":[]}')
        automatic.remove_generated(self.level)
        self.assertTrue(self.sidecar.exists())

    def test_stale_staging_does_not_block_generation(self):
        stale = self.sidecar.with_suffix('.json.part')
        stale.write_text('interrupted write')
        with patch.object(automatic, 'prepare_export', return_value=self.inputs), \
             patch.object(automatic.subprocess, 'run', side_effect=self.generate):
            self.prepare()
        self.assertTrue(self.sidecar.exists())
        self.assertEqual(stale.read_text(), 'interrupted write')
        self.assertFalse(list(self.level.glob('.probe-interiors-*')))

    def test_failed_publish_cleans_staging_and_preserves_previous_sources(self):
        original = '{"generator":"openrac-interiors-v1"}'
        self.sidecar.write_text(original)
        with patch.object(automatic, 'prepare_export', return_value=self.inputs), \
             patch.object(automatic.subprocess, 'run', side_effect=self.generate), \
             patch.object(Path, 'replace', side_effect=OSError('publish failed')):
            with self.assertRaisesRegex(OSError, 'publish failed'):
                self.prepare()
        self.assertEqual(self.sidecar.read_text(), original)
        self.assertFalse(list(self.level.glob('.probe-interiors-*')))

    def test_failed_inference_does_not_publish_sources(self):
        with patch.object(automatic, 'prepare_export', return_value=self.inputs), \
             patch.object(automatic.subprocess, 'run', side_effect=subprocess.CalledProcessError(1, ['infer'])):
            with self.assertRaises(subprocess.CalledProcessError):
                self.prepare()
        self.assertFalse(self.sidecar.exists())

    def test_failed_inference_keeps_report_and_native_diagnostics(self):
        report_path = self.output/'interior-inference.json'
        report_path.write_text('previous verified report')

        def fail(command, **kwargs):
            kwargs['stdout'].write('native inference failure fixture\n')
            Path(command[2]).write_text('partial report')
            raise subprocess.CalledProcessError(1, command)

        with patch.object(automatic, 'prepare_export', return_value=self.inputs), \
             patch.object(automatic.subprocess, 'run', side_effect=fail):
            with self.assertRaises(automatic.diagnostics.NativeFailure) as caught:
                self.prepare()
        self.assertIn('native inference failure fixture', caught.exception.output)
        self.assertEqual(report_path.read_text(), 'previous verified report')
        self.assertFalse(self.sidecar.exists())
        self.assertFalse(list(self.output.glob('.interior-inference-*')))

    def test_non_gameplay_scenes_skip_inference(self):
        with patch.object(automatic, 'prepare_export') as read:
            automatic.prepare(self.level, self.output, self.baker, self.manifest, None)
        read.assert_not_called()

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
