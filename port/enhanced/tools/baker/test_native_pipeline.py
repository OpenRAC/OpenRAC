# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Headless native launcher-flow regressions using synthetic exported rooms, never retail data."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import struct
import sys
import tempfile
import unittest
import zlib

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
            manifest['level'] = 8  # A gameplay level without shipped emission or water recipes.
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

    def test_log_build_identity_survives_packaging_and_appended_retries(self):
        """Main and per-level logs identify the compiled baker and each calling launcher."""
        prefix = 'Probe build context: '
        for version in ('0.9-test', '0.10-test'):
            env = dict(os.environ, OPENRAC_LAUNCHER_VERSION=version)
            result = subprocess.run(self.command(), capture_output=True, text=True, env=env, timeout=40)
            self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
            context = json.loads(next(s[len(prefix):] for s in result.stdout.splitlines() if s.startswith(prefix)))
            self.assertEqual(context['launcher_version'], version)
            self.assertTrue(context['baker_version'])
            self.assertTrue(context['baker_commit'])
        text = (self.cache/'probe-bake.log').read_text(encoding='utf-8')
        contexts = [json.loads(s[len(prefix):]) for s in text.splitlines() if s.startswith(prefix)]
        self.assertEqual([c['launcher_version'] for c in contexts], ['0.9-test', '0.10-test'])
        for index in range(2):
            log = (self.cache/f'level_{index:02}'/'batch.log').read_text(encoding='utf-8')
            self.assertIn(prefix, log)
            self.assertIn('0.9-test', log)

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

    def test_modded_placement_bake_matches_runtime_and_invalidates_on_edit(self):
        level = self.levels[0]
        manifest = json.loads((level/'manifest.json').read_text())
        manifest['classes']['tie']['1'] = {'mesh': 'collision.glb'}
        (level/'manifest.json').write_text(json.dumps(manifest))
        mods = self.root/'mods'
        override = mods/'level_00'
        packed = self.root/'packed'
        packed_level = packed/'level_00'
        packed_level.mkdir(parents=True)
        (packed/'packed.json').write_text('{"version":"ntsc"}')
        matrix = [0.1,0,0,0, 0,0.1,0,0, 0,0,0.1,0, 1,0,1,1]
        placements = {'ties': [{'class': 1, 'matrix': matrix, 'colours': [0x80808080]*64}]}

        def save_edit():
            (packed_level/'placements.json').write_text(json.dumps(placements))
            stacked = subprocess.run(
                [sys.executable, str(ROOT/'editor/overrides.py'), 'stack', '--into', str(self.root/'packs'),
                 '--levels', str(self.exports), '--packed', str(packed), '--out', str(mods)],
                capture_output=True, text=True, timeout=20)
            self.assertEqual(stacked.returncode, 0, stacked.stdout + stacked.stderr)

        original = self.cache/'level_00'/'sparse.orpg'

        def check(asset, *args):
            return subprocess.run(
                [str(BAKER), '--level-dir', str(level), '--verify', str(asset),
                 '--game-rounding', '--rays', '16', '--bounces', '0', '--compression', 'none',
                 *map(str, args)], capture_output=True, text=True, timeout=20)

        def variant(path):
            key = subprocess.check_output([str(BAKER), '--print-mod-key', str(path)], text=True).strip()
            return self.cache/'variants'/key/'level_00'

        self.run_bake('--compression', 'none')
        self.assertEqual(check(original).returncode, 0)
        untouched = original.read_bytes()
        save_edit()
        self.assertIn('stale probe scene', check(original, '--level-overrides', override).stderr)
        result = self.run_bake('--mods-dir', mods, '--compression', 'none')
        self.assertIn('level_00: baked', result.stdout)
        self.assertIn('level_01: up to date', result.stdout)
        baked = variant(override)
        edited_asset = baked/'sparse.orpg'
        edited = check(edited_asset, '--level-overrides', override)
        self.assertEqual(edited.returncode, 0, edited.stdout + edited.stderr)
        self.assertEqual(original.read_bytes(), untouched)
        self.assertIn('stale probe scene', check(edited_asset).stderr)
        stamp = json.loads((baked/'openrac-bake.json').read_text())
        self.assertTrue(stamp['modded'])
        source = [str(BAKER), '--print-modded-source', str(level), str(override)]
        self.assertEqual(subprocess.check_output(source, text=True).strip(), stamp['source'])
        self.assertEqual(self.run_bake('--mods-dir', mods, '--compression', 'none')
                         .stdout.count('up to date'), 2)
        matrix[12] = 2
        save_edit()
        self.assertNotEqual(subprocess.check_output(source, text=True).strip(), stamp['source'])
        self.assertIn('stale probe scene', check(edited_asset, '--level-overrides', override).stderr)
        self.assertNotEqual(variant(override), baked)
        self.assertEqual(self.run_bake('--compression', 'none').stdout.count('up to date'), 2)

        destination = mods/'destinations'/'test-ship'/'level_00'
        shutil.copytree(override, destination)
        (mods/'destinations.json').write_text(json.dumps({'destinations': [{'id': 'test-ship', 'level': 0}]}))
        result = self.run_bake('--mods-dir', mods, '--compression', 'none')
        self.assertIn('test-ship/level_00: baked', result.stdout)
        asset = variant(destination)/'sparse.orpg'
        self.assertNotEqual(asset, variant(override)/'sparse.orpg')
        verified = check(asset, '--level-overrides', destination)
        self.assertEqual(verified.returncode, 0, verified.stdout + verified.stderr)
        self.assertEqual(self.run_bake('--mods-dir', mods, '--compression', 'none')
                         .stdout.count('up to date'), 3)

    def test_packed_gameplay_changes_lighting_variant(self):
        level = self.levels[0]
        override = self.root/'mods'/'level_00'
        override.mkdir(parents=True)
        wad = override/'gameplay_ntsc.wad'
        wad.write_bytes(b'first gameplay')
        source = [str(BAKER), '--print-modded-source', str(level), str(override)]
        key = [str(BAKER), '--print-mod-key', str(override)]
        before = [subprocess.check_output(command) for command in (source, key)]
        wad.write_bytes(b'edited gameplay')
        self.assertTrue(all(subprocess.check_output(command) != value
                            for command, value in zip((source, key), before)))

    def test_bake_uses_prepared_gameplay_and_collision(self):
        level = self.levels[0]
        (level/'probe-bake.json').write_text(json.dumps({
            'version': 1, 'regions': {'enabled': False, 'interior_lighting': True},
        }))
        mods = self.root/'mods'
        override = mods/'level_00'
        inputs = override/'bake-inputs'
        inputs.mkdir(parents=True)
        gameplay = (level/'bake-inputs/gameplay.bin').read_bytes() + bytes(16)
        collision = bytearray((level/'collision.glb').read_bytes())
        json_length, = struct.unpack_from('<I', collision, 12)
        struct.pack_into('<f', collision, 28 + json_length, -7.5)
        (override/'gameplay_ntsc.wad').write_bytes(b'packed gameplay')
        (override/'data_ntsc.bin').write_bytes(b'packed collision')
        (inputs/'gameplay.bin').write_bytes(gameplay)
        (inputs/'collision.glb').write_bytes(collision)

        self.run_bake('--mods-dir', mods, '--compression', 'none')
        key = subprocess.check_output([str(BAKER), '--print-mod-key', str(override)], text=True).strip()
        report = self.cache/'variants'/key/'level_00'/'interior-input.json'
        self.assertEqual(json.loads(report.read_text())['source_sha256'],
                         hashlib.sha256(gameplay + collision).hexdigest())

    def test_lz4_storage_is_opt_in_and_part_of_the_cache_identity(self):
        check = subprocess.run(self.command('--lz4-assets', '--dry-run'),
                               capture_output=True, text=True, timeout=40)
        if '--lz4-assets requires a build with OPENRAC_BAKE_LZ4' in check.stdout + check.stderr:
            self.skipTest('baker was built without LZ4')
        self.assertEqual(check.returncode, 0, check.stdout + check.stderr)

        self.run_bake('--compression', 'none')
        path = self.cache/'level_00'/'sparse.orpg'
        raw = path.read_bytes()
        self.assertEqual(raw[:4], b'ORGP')

        self.run_bake('--compression', 'none', '--lz4-assets')
        packed = path.read_bytes()
        self.assertEqual(packed[:4], bytes.fromhex('04 22 4d 18'))
        self.assertLess(len(packed), len(raw))
        self.assertEqual(self.run_bake('--compression', 'none', '--lz4-assets')
                         .stdout.count('up to date'), 2)

        self.run_bake('--compression', 'none')
        self.assertEqual(path.read_bytes(), raw)

    def test_refreshes_shipped_lighting_profiles_without_replacing_local_edits(self):
        profiles = self.root/'profiles'
        tracked = profiles/'rac1-ntsc/level_08'
        tracked.mkdir(parents=True)
        name = 'lighting-environments.json'
        profile = {'version': 1, 'barriers': [], 'sources': []}
        (tracked/name).write_text(json.dumps(profile))
        self.run_bake('--profiles-dir', profiles)
        self.assertEqual((self.levels[0]/name).read_bytes(), (tracked/name).read_bytes())
        self.assertIn('up to date', self.run_bake('--profiles-dir', profiles).stdout)

        local_edit = json.dumps(profile, indent=3)
        (self.levels[1]/name).write_text(local_edit)
        (tracked/name).write_text(json.dumps(profile, indent=2))
        result = self.run_bake('--profiles-dir', profiles)
        self.assertIn('level_00: baked', result.stdout)
        self.assertEqual((self.levels[0]/name).read_bytes(), (tracked/name).read_bytes())
        self.assertEqual((self.levels[1]/name).read_text(), local_edit)
        self.assertEqual(self.run_bake('--profiles-dir', profiles).stdout.count('up to date'), 2)

    def test_interior_authoring_rejects_zero_area_barriers(self):
        level = self.levels[0]
        (level/'lighting-environments.json').write_text(json.dumps({
            'version': 1, 'barriers': [[[0, 0, 0], [1, 1, 1], [2, 2, 2]]], 'sources': [],
        }))
        output = self.root/'inference'
        output.mkdir()
        result = subprocess.run([str(BAKER), '--prepare-level-interiors', str(level),
            str(level/'bake-inputs/gameplay.bin'), str(output)], capture_output=True, text=True,
            timeout=20)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('barrier 0 has zero area', result.stderr)
        self.assertFalse((level/'probe-interiors.json').exists())

    def test_parallel_emission_hashing_matches_serial_preparation(self):
        profiles = self.root/'profiles'
        shutil.copytree(ROOT/'port/enhanced/lighting/profiles', profiles)
        level = self.levels[0]
        manifest = json.loads((level/'manifest.json').read_text())
        directory = profiles/(manifest['game']+'-'+manifest['version'])
        # Hash matching needs no scene material, but the fixture PNG remains independently valid.
        def chunk(kind, data):
            return struct.pack('>I', len(data))+kind+data+struct.pack('>I', zlib.crc32(kind+data))
        source = level/'textures'/'emission-fixture.png'
        source.parent.mkdir(exist_ok=True)
        source.write_bytes(b'\x89PNG\r\n\x1a\n'
            +chunk(b'IHDR', struct.pack('>IIBBBBB', 1, 1, 8, 6, 0, 0, 0))
            +chunk(b'IDAT', zlib.compress(b'\x00\xff\xff\xff\xff'))+chunk(b'IEND', b''))
        for index in range(24):
            shutil.copyfile(source, level/'textures'/f'hash-{index:02}.png')
        catalog = directory/'material-overrides.json'
        catalog.write_text(json.dumps({'version':1,
            'presets':{'panel':{'emission':{'runtime_intensity':1,'bake_intensity':2}}},
            'overrides':[{'id':'panel','match':{'texture_file_sha256':hashlib.sha256(source.read_bytes()).hexdigest()},'preset':'panel'}]}))
        defaults = directory/f"level_{manifest['level']:02}"/'probe-emission.json'
        serial = self.root/'serial-emission'
        shutil.copytree(level, serial)
        result = subprocess.run([str(BAKER), '--prepare-level-emission', str(serial),
            str(catalog), str(defaults)], capture_output=True, text=True, timeout=20)
        self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
        parallel = self.root/'parallel-emission'
        shutil.copytree(level, parallel)
        result = subprocess.run([str(BAKER), '--prepare-level-emission', str(parallel),
            str(catalog), str(defaults), '--threads', '2'], capture_output=True, text=True, timeout=20)
        self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
        self.assertEqual((serial/'probe-emission.json').read_bytes(),
                         (parallel/'probe-emission.json').read_bytes())
        self.run_bake('--profiles-dir', profiles)
        self.assertEqual((serial/'probe-emission.json').read_bytes(),
                         (level/'probe-emission.json').read_bytes())
        generated = json.loads((level/'probe-emission.json').read_text())['_catalog_defaults']
        self.assertTrue(all(f'textures/hash-{index:02}.png' in generated for index in range(24)))

    def test_emission_recipe_composes_local_pixels_and_refreshes_the_cache(self):
        def chunk(kind, data):
            return struct.pack('>I', len(data))+kind+data+struct.pack('>I', zlib.crc32(kind+data))

        def write_row(path, width, colour_type, pixels):
            path.write_bytes(b'\x89PNG\r\n\x1a\n'
                +chunk(b'IHDR', struct.pack('>IIBBBBB', width, 1, 8, colour_type, 0, 0, 0))
                +chunk(b'IDAT', zlib.compress(b'\0'+bytes(pixels)))+chunk(b'IEND', b''))

        def read_rgb_row(path):
            png = path.read_bytes()
            chunks = []
            offset = 8
            while offset < len(png):
                length, kind = struct.unpack_from('>I4s', png, offset)
                if kind == b'IDAT':
                    chunks.append(png[offset+8:offset+8+length])
                offset += length+12
            method, *pixels = zlib.decompress(b''.join(chunks))
            # A single row has no above-neighbour values for the PNG filters.
            for i in range(3, len(pixels)):
                left = pixels[i-3]
                predictor = left if method in (1, 4) else left//2 if method == 3 else 0
                pixels[i] = (pixels[i]+predictor) & 255
            return pixels

        profiles = self.root/'recipe profiles'
        tracked = profiles/'rac1-ntsc/level_08'
        masks = tracked/'emission-masks'
        masks.mkdir(parents=True)
        mask = masks/'light-mask.png'
        write_row(mask, 3, 0, [0, 128, 255])
        (tracked/'material-overrides.json').write_text('{"version":1,"overrides":[]}')
        recipe_path = tracked/'emission-textures.json'
        recipes = {
            'version': 1, 'textures': [{
                'output': 'light-emission.png', 'mask': mask.name, 'mode': 'source',
                'source': 'textures/fixture.png',
            }, {
                'output': 'tinted-emission.png', 'mask': mask.name, 'mode': 'tint',
                'source': 'textures/fixture.png', 'colour': [255, 128, 0],
            }],
        }
        recipe_path.write_text(json.dumps(recipes))
        for level in self.levels:
            folder = level/'textures'
            folder.mkdir(exist_ok=True)
            write_row(folder/'fixture.png', 3, 2, [100, 160, 200]*3)
        self.run_bake('--profiles-dir', profiles)
        generated = self.levels[0]/'materials/level/textures/light-emission.png'
        fingerprint_cmd = [str(BAKER), '--print-emission-fingerprint', str(self.levels[0]), str(profiles)]
        fingerprint = subprocess.check_output(fingerprint_cmd, text=True).strip()
        stamp = json.loads((self.cache/'level_00/openrac-bake.json').read_text())
        self.assertEqual(stamp['emission_textures_sha256'], fingerprint)
        self.assertEqual(read_rgb_row(generated), [0, 0, 0, 50, 80, 100, 100, 160, 200])
        self.assertEqual(read_rgb_row(generated.with_name('tinted-emission.png')),
                         [0, 0, 0, 50, 40, 0, 100, 80, 0])
        self.assertEqual(self.run_bake('--profiles-dir', profiles).stdout.count('up to date'), 2)
        previous = generated.read_bytes()
        generated.unlink()
        self.assertNotEqual(subprocess.run(fingerprint_cmd, capture_output=True).returncode, 0)
        self.assertEqual(self.run_bake('--profiles-dir', profiles).stdout.count('up to date'), 2)
        self.assertEqual(generated.read_bytes(), previous)
        write_row(mask, 3, 0, [255, 0, 255])
        self.assertNotEqual(subprocess.check_output(fingerprint_cmd, text=True).strip(), fingerprint)
        self.assertEqual(self.run_bake('--profiles-dir', profiles).stdout.count(': baked'), 2)
        self.assertEqual(read_rgb_row(generated), [100, 160, 200, 0, 0, 0, 100, 160, 200])
        write_row(self.levels[0]/'textures/fixture.png', 3, 2, [80, 120, 160]*3)
        result = self.run_bake('--profiles-dir', profiles)
        self.assertIn('level_00: baked', result.stdout)
        self.assertIn('level_01: up to date', result.stdout)
        self.assertEqual(read_rgb_row(generated), [80, 120, 160, 0, 0, 0, 80, 120, 160])
        previous = generated.read_bytes()
        write_row(mask, 2, 0, [255, 0])
        self.run_bake('--profiles-dir', profiles, success=False)
        self.assertEqual(generated.read_bytes(), previous)
        write_row(mask, 3, 2, [255, 255, 255]*3)
        result = self.run_bake('--profiles-dir', profiles, success=False)
        self.assertIn('8-bit grayscale PNG', result.stdout)
        write_row(mask, 3, 0, [255, 0, 255])
        recipes['textures'][0]['gian'] = 2
        recipe_path.write_text(json.dumps(recipes))
        result = self.run_bake('--profiles-dir', profiles, success=False)
        self.assertIn('unknown emission recipe field: gian', result.stdout)
        self.assertEqual(generated.read_bytes(), previous)

    def test_compression_is_stored_and_format_changes_invalidate_cache(self):
        self.run_bake('--compression', 'bc6h')
        path = self.cache/'level_00'/'sparse.orpg'
        compressed = path.read_bytes()
        self.assertEqual(struct.unpack_from('<I', compressed, 4)[0], 8)
        # Identity has a variable byte length; depth and colour tags follow the fixed grid header.
        identity_bytes = struct.unpack_from('<I', compressed, 12)[0]
        self.assertEqual(struct.unpack_from('<II', compressed, 96 + identity_bytes), (3, 1))
        self.run_bake('--compression', 'none')
        raw = path.read_bytes()
        self.assertEqual(struct.unpack_from('<II', raw, 96 + identity_bytes), (3, 0))
        self.assertGreater(len(raw), len(compressed))
        self.assertNotEqual(compressed, raw)
        result = self.run_bake('--compression', 'none')
        self.assertEqual(result.stdout.count('up to date'), 2)

    def test_astc_bake_verifies_under_runtime_rounding(self):
        self.run_bake('--compression', 'astc')
        for index in range(2):
            output = self.cache/f'level_{index:02}'
            self.assertTrue(json.loads((output/'bake-result.json').read_text())['verified'])
            self.assertGreater((output/'sparse.orpg').stat().st_size, 0)

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
        """Default captures use their own floor anchors even when local GI seeds are disabled."""
        level = self.levels[0]
        matrix = [1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1]
        (level/'placements.json').write_text(json.dumps(dict(ties=[],shrubs=[],mobys=[
            dict(index=0,**{'class':0},matrix=matrix)])))
        settings = dict(version=1,regions=dict(enabled=False,interior_lighting=False),
            environment_probes=dict(resolution=4,filter_samples=1,maximum_probes=16))
        (level/'probe-bake.json').write_text(json.dumps(settings))
        for strategy, local_regions in (('gameplay', False), ('gameplay', True), ('scene', False)):
            with self.subTest(strategy=strategy, local_regions=local_regions):
                settings['strategy'] = strategy
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
        self.assertIn('up to date', self.run_bake().stdout)
        (output/'environment.orpe').unlink()
        self.assertIn(f'{level.name}: baked', self.run_bake().stdout)
        self.assertTrue((output/'environment.orpe').is_file())

        settings['environment_probes']['enabled'] = False
        settings['regions']['enabled'] = False
        (level/'probe-bake.json').write_text(json.dumps(settings))
        self.run_bake()
        self.assertFalse((output/'environment.orpe').exists())

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
