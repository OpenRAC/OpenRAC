# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Headless native launcher-flow regressions using synthetic exported rooms, never retail data."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
import unittest

import bake_level
import diagnostics
from test_baker_cli import exported_room

BAKER = Path(sys.argv.pop(1)).resolve() if __name__ == '__main__' and len(sys.argv) > 1 else None
ROOT = Path(__file__).resolve().parents[4]


@unittest.skipUnless(BAKER, 'requires the native enhanced baker')
class NativePipeline(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='native pipeline ')
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.exports = self.root/'exports'
        self.exports.mkdir()
        self.cache = self.root/'cache'
        self.levels = []
        for index in range(2):
            parent = self.root/str(index)
            parent.mkdir()
            level, _, _ = exported_room(parent)
            final = self.exports/f'level_{index:02}'
            level.rename(final)
            manifest = json.loads((final/'manifest.json').read_text())
            manifest['level'] = 2  # The room fixture has no real level-specific emitters or water.
            manifest['collision'] = 'collision.glb'  # Normal levels explicitly declare floor geometry.
            (final/'manifest.json').write_text(json.dumps(manifest))
            (final/'probe-bake.json').write_text(json.dumps(dict(version=1,
                regions=dict(enabled=False,interior_lighting=False))))
            self.levels.append(final)

    def command(self, *extra):
        return [str(BAKER),'--bake-all','--levels-dir',str(self.exports),'--output-dir',str(self.cache),
                '--update','--jobs','2','--threads','2','--rays','16','--bounces','0',
                '--profiles-dir',str(ROOT/'port/enhanced/lighting/profiles'),*map(str,extra)]

    def run_bake(self, *extra, success=True):
        result = subprocess.run(self.command(*extra),capture_output=True,text=True,timeout=40)
        if success:
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)
        else:
            self.assertNotEqual(result.returncode,0,result.stdout)
        return result

    def test_native_flow_never_starts_python_for_normal_exports_and_reuses_cache(self):
        (self.levels[0]/'bake-inputs/gameplay.bin').unlink()
        self.run_bake()
        paths = [self.cache/f'level_{i:02}'/'sparse.orpg' for i in range(2)]
        before = [p.stat().st_mtime_ns for p in paths]
        result = self.run_bake()
        self.assertEqual(before,[p.stat().st_mtime_ns for p in paths])
        self.assertEqual(result.stdout.count('up to date'),2)
        records = json.loads((self.cache/'bake-all-result.json').read_text())['levels']
        self.assertTrue(all(row['verified'] and row['kept'] for row in records))
        (self.levels[0]/'placements.json').write_text('{"ties":[],"shrubs":[],"mobys":[]}\n')
        self.run_bake()
        self.assertNotEqual(before[0],paths[0].stat().st_mtime_ns)
        self.assertEqual(before[1],paths[1].stat().st_mtime_ns)

    def test_missing_retained_gameplay_requires_reexport_without_running_python(self):
        level = self.levels[0]
        (level/'bake-inputs/gameplay.bin').unlink()
        (level/'probe-bake.json').write_text('{"version":1,"regions":{"enabled":false,"interior_lighting":true}}')
        result = self.run_bake(success=False)
        self.assertIn('re-export the level', result.stdout)
        self.assertNotIn('cannot start', result.stdout)

    def test_native_assets_and_reports_match_the_python_reference_flow(self):
        self.run_bake()
        for index, level in enumerate(self.levels):
            reference = self.root/f'reference-{index}'
            bake_level.bake_level(level,reference,BAKER,options=['--rays','16','--bounces','0','--threads','2'])
            actual = self.cache/level.name
            self.assertEqual((reference/'sparse.orpg').read_bytes(),(actual/'sparse.orpg').read_bytes())
            for name in ('seed-plan.json','regions.json','sparse.orpg.regions.json','bake-result.json'):
                self.assertEqual(json.loads((reference/name).read_text()),json.loads((actual/name).read_text()))

    def test_failed_rebuild_preserves_previous_lighting_and_continues_other_levels(self):
        self.run_bake()
        before = (self.cache/'level_00/sparse.orpg').read_bytes()
        (self.cache/'keep.txt').write_text('private')
        (self.levels[0]/'collision.glb').write_bytes(b'invalid geometry')
        result = self.run_bake(success=False)
        self.assertIn('openrac-bake-error:',result.stdout)
        self.assertEqual(before,(self.cache/'level_00/sparse.orpg').read_bytes())
        self.assertTrue((self.cache/'.level_00.partial/batch.log').is_file())
        self.assertEqual((self.cache/'keep.txt').read_text(),'private')
        records = json.loads((self.cache/'bake-all-result.json').read_text())['levels']
        self.assertFalse(records[0]['verified'])
        self.assertTrue(records[1]['verified'])

    def test_resume_and_interrupted_installation_retain_verified_grids(self):
        self.run_bake()
        final = self.cache/'level_00'
        before = (final/'sparse.orpg').read_bytes()
        partial = self.cache/'.level_00.partial'
        final.rename(partial)
        shutil.copyfile(partial/'openrac-bake.json',partial/'openrac-bake.partial.json')
        result = self.run_bake()
        self.assertIn('Kept sparse.orpg',result.stdout)
        self.assertEqual(before,(final/'sparse.orpg').read_bytes())
        modified = (final/'sparse.orpg').stat().st_mtime_ns
        final.rename(self.cache/'.level_00.previous')
        result = self.run_bake()
        self.assertEqual(result.stdout.count('up to date'),2)
        self.assertEqual(modified,(final/'sparse.orpg').stat().st_mtime_ns)

    @unittest.skipUnless(os.name == 'nt', 'Windows sharing locks prevent directory moves')
    def test_persistent_installation_lock_restores_previous_lighting_then_retry_resumes(self):
        self.run_bake()
        final = self.cache/'level_00'
        previous = (final/'sparse.orpg').read_bytes()
        (self.levels[0]/'placements.json').write_text('{"ties":[],"shrubs":[],"mobys":[]}\n')
        (self.levels[0]/'probe-bake.json').write_text('{"version":1,"regions":{"enabled":false,"interior_lighting":true}}')
        process = subprocess.Popen(self.command('--jobs','1'),stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
        try:
            for line in process.stdout:
                if 'Probe batch ready' in line:
                    break
            partial = self.cache/'.level_00.partial'
            # The final directory can move; only the replacement remains locked during its install.
            with (partial/'emission-audit.json').open('rb'):
                remaining = process.communicate(timeout=30)[0]
            self.assertNotEqual(process.returncode,0,remaining)
        finally:
            if process.poll() is None: process.kill();process.wait()
            process.stdout.close()
        self.assertEqual(previous,(final/'sparse.orpg').read_bytes())
        self.assertFalse((self.cache/'.level_00.previous').exists())
        result = self.run_bake()
        self.assertIn('Kept sparse.orpg',result.stdout)

    def test_reflections_do_not_require_local_gi_or_generated_interior_lights(self):
        level = self.levels[0]
        matrix = [1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1]
        (level/'placements.json').write_text(json.dumps(dict(ties=[],shrubs=[],mobys=[
            dict(index=0,**{'class':0},matrix=matrix)])))
        settings = dict(version=1,regions=dict(enabled=False,interior_lighting=False),
            environment_probes=dict(enabled=True,resolution=4,filter_samples=1,maximum_probes=16))
        (level/'probe-bake.json').write_text(json.dumps(settings))
        for local_regions in (False, True):
            with self.subTest(local_regions=local_regions):
                settings['regions'].update(enabled=local_regions,no_moby_seeds=True)
                settings['dense_spacing'] = [4,4,4] if local_regions else [256,256,256]
                (level/'probe-bake.json').write_text(json.dumps(settings))
                self.run_bake()
                output = self.cache/level.name
                self.assertTrue((output/'environment.orpe').is_file())
                self.assertFalse((level/'probe-interiors.json').exists())
                self.assertEqual(bool(json.loads((output/'regions.json').read_text())['regions']),local_regions)
                evidence = json.loads((output/'seed-plan.json').read_text())
                self.assertTrue(evidence.get('floor_anchors',evidence['moby_seeds']))

    def test_background_scene_without_collision_skips_gameplay_floor_analysis(self):
        level = self.levels[0]
        (level/'collision.glb').rename(level/'terrain.glb')
        manifest = json.loads((level/'manifest.json').read_text())
        manifest.update(level=99,terrain='terrain.glb',collision=None)
        (level/'manifest.json').write_text(json.dumps(manifest))
        (level/'probe-bake.json').write_text('{"version":1}')
        self.run_bake()
        output = self.cache/level.name
        self.assertTrue((output/'sparse.orpg').is_file())
        self.assertFalse((output/'interior-inference.json').exists())
        self.assertEqual(json.loads((output/'seed-plan.json').read_text())['regions'],[])

    def test_scene_strategy_bakes_one_geometry_bounded_volume_without_gameplay(self):
        """Scene placement retains transport without collision floors, local GI or reflection anchors."""
        level = self.levels[0]
        (level/'bake-inputs/gameplay.bin').unlink()
        manifest = json.loads((level/'manifest.json').read_text())
        settings = dict(version=1,strategy='scene',sparse_spacing=[8,8,8],
            regions=dict(enabled=True,interior_lighting=True),environment_probes=dict(enabled=True))
        (level/'probe-bake.json').write_text(json.dumps(settings))
        for collision in (None, 'collision.glb'):
            with self.subTest(collision=collision):
                manifest['collision'] = collision
                (level/'manifest.json').write_text(json.dumps(manifest))
                self.run_bake()
                output = self.cache/level.name
                self.assertTrue((output/'sparse.orpg').is_file())
                self.assertFalse(list(output.glob('region-*.orpg')))
                self.assertFalse((output/'environment.orpe').exists())
                self.assertFalse((output/'interior-inference.json').exists())
                self.assertEqual(json.loads((output/'regions.json').read_text())['regions'],[])
                plan = json.loads((output/'seed-plan.json').read_text())
                self.assertEqual(plan['bake_profile']['strategy'],'scene')
                fields = dict(line.split(' ',1) for line in (output/'sparse.orpg.txt').read_text().splitlines() if ' ' in line)
                origin = list(map(float,fields['probe_origin'].split()))
                dimensions = list(map(int,fields['probe_dimensions'].split()))
                for axis in range(3):
                    self.assertLess(origin[axis],manifest['bounds'][0][axis])
                    self.assertGreater(origin[axis]+8*(dimensions[axis]-1),manifest['bounds'][1][axis])
        self.assertEqual(self.run_bake().stdout.count('up to date'),2)

    def test_invalid_strategy_is_rejected_before_publishing(self):
        """Misspelled or mistyped strategies cannot silently select a different bake path."""
        for strategy in ('background', False, 1):
            with self.subTest(strategy=strategy):
                (self.levels[0]/'probe-bake.json').write_text(json.dumps(dict(version=1,strategy=strategy)))
                result = self.run_bake(success=False)
                self.assertIn('strategy must be gameplay or scene',result.stdout)
                self.assertFalse((self.cache/'level_00').exists())

    def test_cache_lock_is_shared_with_the_legacy_python_flow(self):
        self.cache.mkdir()
        with diagnostics.cache_lock(self.cache):
            result = self.run_bake(success=False)
        self.assertIn('busy',result.stdout)
        self.run_bake()

    def test_invalid_controls_dry_run_and_recursive_outputs_do_not_modify_exports(self):
        command = self.command('--dry-run')
        result = subprocess.run(command,capture_output=True,text=True,timeout=10)
        self.assertEqual(result.returncode,0,result.stdout)
        self.assertFalse(self.cache.exists())
        for arguments in (('--jobs','257'),('--rays','0'),('--bounces','4294967295'),
                          ('--output-dir',self.exports/'bad'),('--python','removed')):
            self.run_bake(*arguments,success=False)
        self.assertFalse(self.cache.exists())
        self.assertFalse((self.exports/'bad').exists())

    def test_water_decoding_and_numerics_run_in_the_automatic_native_flow(self):
        sys.path.insert(0,str(ROOT/'port/enhanced/tools/water'))
        from test_prepare_water import overlay
        level = self.levels[0]
        manifest = json.loads((level/'manifest.json').read_text())
        manifest.update(level=1,bake_inputs=dict(gameplay='rac1-v1',water='profile-v1'))
        (level/'manifest.json').write_text(json.dumps(manifest))
        (level/'bake-inputs/overlay.bin').write_bytes(overlay())
        settings = json.loads((level/'probe-bake.json').read_text())
        settings['water_sources'] = json.loads((ROOT/'port/enhanced/lighting/profiles/rac1-ntsc/level_01/probe-bake.json').read_text())['water_sources']
        (level/'probe-bake.json').write_text(json.dumps(settings))
        profiles = self.root/'empty profiles'
        profiles.mkdir()
        self.run_bake('--profiles-dir',profiles)
        self.assertTrue((level/'water.bin').is_file())
        self.assertTrue((level/'water.mesh').is_file())
        self.assertEqual((level/'water.bin').read_bytes()[20:52],(level/'water.mesh').read_bytes()[16:48])
        before = [(level/name).stat().st_mtime_ns for name in ('water.bin','water.mesh')]
        self.run_bake('--profiles-dir',profiles)
        self.assertEqual(before,[(level/name).stat().st_mtime_ns for name in ('water.bin','water.mesh')])
        # Old Python stamps are never adopted, even if they claim the same native input hashes.
        path = level/'water-bake.json'
        old = json.loads(path.read_text())
        old['version'] = 1
        path.write_text(json.dumps(old))
        self.run_bake('--profiles-dir',profiles)
        self.assertEqual(json.loads(path.read_text())['version'],3)
        self.assertNotEqual(before,[(level/name).stat().st_mtime_ns for name in ('water.bin','water.mesh')])
        previous = [(level/name).read_bytes() for name in ('water.bin','water.mesh')]
        settings['water_sources']['tables'][1]['pointer_offset'] = 999
        (level/'probe-bake.json').write_text(json.dumps(settings))
        self.run_bake('--profiles-dir',profiles,success=False)
        self.assertEqual(previous,[(level/name).read_bytes() for name in ('water.bin','water.mesh')])



if __name__ == '__main__':
    unittest.main()
