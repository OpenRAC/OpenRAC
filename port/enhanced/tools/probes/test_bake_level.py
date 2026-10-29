# SPDX-License-Identifier: GPL-3.0-or-later
import json
import io
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

import bake_level as batch
import seed_regions as seeds
from test_seed_regions import gameplay


INSPECTION = '''game_version rac1-ntsc
level 1
probe_origin 0 0 0
probe_spacing 32 32 32
probe_dimensions 5 5 5
emissive_triangles 20
automatic_liquid_triangles 0
point_emitters 2
scene_hash 42
'''


class LevelBake(unittest.TestCase):
    def test_adaptive_density_settings_are_explicit_and_bounded(self):
        self.assertFalse(batch.RegionSettings().adaptive_density)
        for fields in ({'adaptive_density': 1}, {'adaptive_relative_error': .3},
                       {'adaptive_absolute_error': -.1}, {'adaptive_absolute_error': .2}):
            (self.level/'probe-bake.json').write_text(json.dumps({'version': 1, 'regions': fields}))
            with self.assertRaises(ValueError):
                batch.RegionSettings.load(self.level)
        (self.level/'probe-bake.json').write_text(json.dumps({
            'version': 1, 'regions': {'adaptive_density': True, 'adaptive_relative_error': .08}}))
        self.assertTrue(batch.RegionSettings.load(self.level).adaptive_density)
        self.assertEqual(batch.RegionSettings.load(self.level).adaptive_relative_error, .08)

    def test_local_region_bounds_include_padding_and_scene_mode_is_unchanged(self):
        regions = [{'bounds': [1, 2, 3, 4, 5, 6]}, {'bounds': [-2, 4, 1, 8, 9, 10]}]
        settings = batch.RegionSettings(coarse_bounds='local_regions', coarse_padding=2)
        self.assertEqual(batch.coarse_bounds(regions, settings), [-4, 0, -1, 10, 11, 12])
        self.assertIsNone(batch.coarse_bounds(regions, batch.RegionSettings(coarse_bounds='scene')))
        self.assertIsNone(batch.coarse_bounds([], settings))
        self.assertEqual(batch.RegionSettings().coarse_bounds, 'local_regions')
        self.assertEqual(batch.RegionSettings().coarse_padding, 128)
        self.assertTrue(batch.RegionSettings().interior_lighting)

    def test_coarse_bounds_mode_and_padding_are_validated(self):
        for fields in ({'coarse_bounds': 'invalid'}, {'coarse_bounds': 1}, {'coarse_padding': -1}):
            (self.level/'probe-bake.json').write_text(json.dumps({'version': 1, 'regions': fields}))
            with self.assertRaises(ValueError):
                batch.RegionSettings.load(self.level)

    def test_planning_settings_share_the_transport_profile(self):
        (self.level/'probe-bake.json').write_text(json.dumps({
            'version': 1, 'sky_intensity': .5, 'regions': {'padding': 12, 'max_regions': 512}}))
        settings = batch.RegionSettings.load(self.level)
        self.assertEqual(settings.padding, 12)
        self.assertEqual(settings.max_regions, 512)
        self.assertEqual(settings.coarse_bounds, 'local_regions')
        self.assertEqual(settings.coarse_padding, 128)
        (self.level/'probe-bake.json').write_text('{"version":1,"regions":[]}')
        with self.assertRaises(ValueError):
            batch.RegionSettings.load(self.level)

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.level = self.root/'level'
        self.level.mkdir()
        self.manifest = {'game': 'rac1', 'version': 'ntsc', 'level': 1,
                         'bake_inputs': {'gameplay': 'rac1-v1'}}
        (self.level/'manifest.json').write_text(json.dumps(self.manifest))
        (self.level/'bake-inputs').mkdir()
        (self.level/'bake-inputs/gameplay.bin').write_bytes(gameplay([1]))
        (self.level/'collision.glb').write_bytes(b'synthetic collision placeholder')
        (self.level/'placements.json').write_text(json.dumps({'mobys': [
            {'index': 7, 'class': 500, 'matrix': [0]*12+[40, 40, 60, 1]}]}))
        self.profile = {'dense_spacing': [4, 4, 4], 'probe_format_version': 4, 'bake_version': 1}
        self.output = self.root/'bake'
        self.calls = []
        self.failure = None
        self.changed = False

    def run_native(self, command, **kwargs):
        self.calls.append(command)
        owner = self

        class Input(io.StringIO):
            def close(self):
                if not self.closed and process.returncode is None:
                    owner.sent_plan = json.loads(self.getvalue())
                    output = Path(command[command.index('--batch-output')+1])
                    names = ['sparse'] + [f'region-{i:03d}' for i in range(len(owner.sent_plan['regions']))]
                    for name in names:
                        (output/(name+'.orpg')).write_bytes(b'fake native output')
                    process.returncode = int(bool(owner.failure or owner.changed))
                super().close()

        class Process:
            def __init__(self):
                self.returncode = None
                self.terminated = False
                result = ('source level changed\n' if owner.changed else 'verification failed\n') \
                    if owner.failure or owner.changed else 'Probe batch complete\n'
                self.stdout = io.StringIO(INSPECTION+'Probe batch ready\n'+result)
                self.stdin = Input()

            def wait(self, **kwargs):
                return self.returncode

            def poll(self):
                return self.returncode

            def terminate(self):
                self.terminated = True
                self.returncode = -15

            kill = terminate

        process = Process()
        self.native = process
        return process

    def bake(self):
        # Exercise real planning/registration and command propagation; only native
        # transport and collision decoding are replaced with synthetic fixtures.
        with patch.object(batch.subprocess, 'check_output', return_value=json.dumps(self.profile)), \
             patch.object(batch.automatic_interiors, 'prepare') as interiors, \
             patch.object(batch.emission_audit, 'audit'), \
             patch.object(batch.subprocess, 'Popen', side_effect=self.run_native), \
             patch('moby_seeds.Ground.read_triangles', return_value=[
                 [[30, 30, 75], [50, 30, 75], [30, 70, 75]]]):
            result = batch.bake_level(self.level, self.output, Path('fake-baker'), options=['--rays', '16'])
            self.interiors_called = interiors.called
            return result

    def test_complete_bake_plans_geometry_and_registers_verified_regions(self):
        sparse = self.bake()
        self.assertTrue(self.interiors_called)
        plan = json.loads((self.output/'seed-plan.json').read_text())
        self.assertTrue(plan['regions'])
        capacity = sum(((r['probe_count']+63)//64)*64 for r in plan['regions'])
        self.assertEqual(plan['dense_resident_bytes'], capacity*(12*8*8+8))
        self.assertFalse(plan['dense_residency_budget_exceeded'])
        self.assertEqual(plan['authored_count'], 1)
        self.assertEqual(plan['moby_seeds'][0]['class'], 500)
        # A lone overhead triangle is not evidence of an enclosed room.
        self.assertFalse(any(item['changed'] for item in plan['geometry_coverage']))
        result = json.loads((self.output/'bake-result.json').read_text())
        self.assertTrue(result['verified'])
        self.assertEqual(result['probe_format_version'], 4)
        self.assertEqual(result['bake_version'], 1)
        registry = json.loads(Path(str(sparse)+'.regions.json').read_text())
        self.assertEqual(registry['regions'], result['regions'])
        self.assertEqual(len(registry['regions']), len(plan['regions']))
        self.assertEqual(len(self.calls), 1)
        self.assertIn('--batch-output', self.calls[0])
        self.assertEqual(len(self.sent_plan['regions']), len(plan['regions']))
        self.assertEqual(self.sent_plan['scene_hash'], '42')
        self.assertEqual([r['lattice_bounds'] for r in self.sent_plan['regions']],
                         [r['lattice_bounds'] for r in plan['regions']])
        self.assertEqual(plan['region_repartition']['border_cells'], 1)
        self.assertTrue(all(len(region['spacing']) == 3 and region['max_distance'] > 0
                            for region in self.sent_plan['regions']))
        self.assertTrue(all('--rays' in call for call in self.calls))
        self.assertTrue(self.native.stdin.closed and self.native.stdout.closed)

    def test_failed_dense_verification_never_registers_partial_batch(self):
        self.failure = True
        with self.assertRaises(subprocess.CalledProcessError):
            self.bake()
        self.assertTrue((self.output/'seed-plan.json').exists())
        self.assertFalse((self.output/'regions.json').exists())
        self.assertFalse((self.output/'sparse.orpg.regions.json').exists())
        self.assertFalse((self.output/'bake-result.json').exists())

    def test_planning_failure_terminates_the_prepared_native_process(self):
        with patch.object(seeds, 'plan_level', side_effect=ValueError('invalid seed plan')):
            with self.assertRaisesRegex(ValueError, 'invalid seed plan'):
                self.bake()
        self.assertTrue(self.native.terminated)
        self.assertTrue(self.native.stdin.closed and self.native.stdout.closed)
        self.assertFalse((self.output/'sparse.orpg.regions.json').exists())

    def test_cli_forwards_thread_limit(self):
        with patch('sys.argv', ['bake_level.py', '--level-dir', str(self.level),
                               '--output-dir', str(self.output), '--threads', '2']), \
             patch.object(batch, 'bake_level') as bake:
            batch.main()
        self.assertEqual(bake.call_args.kwargs['options'], ['--threads', '2'])

    def test_source_change_never_registers(self):
        self.changed = True
        with self.assertRaises(subprocess.CalledProcessError):
            self.bake()
        self.assertFalse((self.output/'sparse.orpg.regions.json').exists())

    def test_sparse_only_level_does_not_require_gameplay(self):
        (self.level/'probe-bake.json').write_text(
            '{"version":1,"regions":{"enabled":false,"interior_lighting":false}}')
        (self.level/'bake-inputs/gameplay.bin').unlink()
        sparse = self.bake()
        self.assertEqual(json.loads(Path(str(sparse)+'.regions.json').read_text()), {'regions': []})
        self.assertEqual(self.sent_plan['regions'], [])
        self.assertFalse(self.interiors_called)

    def test_sparse_only_can_still_generate_interior_lighting(self):
        (self.level/'probe-bake.json').write_text('{"version":1,"regions":{"enabled":false}}')
        self.bake()
        self.assertTrue(self.interiors_called)
        self.assertEqual(self.sent_plan['regions'], [])

    def test_existing_outputs_and_recursive_sources_rejected(self):
        self.output.mkdir()
        (self.output/'keep').write_text('previous experiment')
        with self.assertRaisesRegex(ValueError, 'new output directory'):
            self.bake()
        self.assertEqual((self.output/'keep').read_text(), 'previous experiment')
        self.output = self.level/'recursive-output'
        with self.assertRaisesRegex(ValueError, 'outside the source'):
            self.bake()

    def test_missing_gameplay_fails_before_baking(self):
        manifest = {key: value for key, value in self.manifest.items() if key != 'bake_inputs'}
        with self.assertRaisesRegex(ValueError, 're-extract'):
            seeds.read_gameplay(self.level, manifest)
        manifest['bake_inputs'] = {'gameplay': 'unknown'}
        with self.assertRaisesRegex(ValueError, 'unsupported'):
            seeds.read_gameplay(self.level, manifest)

    def test_full_runtime_region_capacity(self):
        self.assertEqual(batch.RegionSettings.load(self.level).max_regions, 1024)
        (self.level/'probe-bake.json').write_text(json.dumps({'version': 1, 'regions': {'max_regions': 1024}}))
        self.assertEqual(batch.RegionSettings.load(self.level).max_regions, 1024)

    def test_invalid_level_planning_settings(self):
        for fields in ({'max_regions': 1025}, {'max_probes': 32}, {'enabled': 1},
                       {'geometry_max_rise': 0}, {'padding': float('nan')}, {'typo': 1}):
            (self.level/'probe-bake.json').write_text(json.dumps({'version': 1, 'regions': fields}))
            with self.assertRaises(ValueError):
                batch.RegionSettings.load(self.level)
