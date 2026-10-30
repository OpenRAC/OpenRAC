# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Fast declarative water decoder regressions from synthetic source tables, never retail assets."""
import copy
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest

from test_baker_cli import exported_room

BAKER = Path(sys.argv.pop(1)).resolve() if __name__ == '__main__' and len(sys.argv) > 1 else None
ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT/'port/enhanced/tools/water'))
import water_sources
from test_prepare_water import overlay


@unittest.skipUnless(BAKER, 'requires the native enhanced baker')
class WaterSources(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix='water source ')
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.level, _, self.output = exported_room(self.root)
        self.profile = json.loads((ROOT/'port/enhanced/lighting/profiles/rac1-ntsc/level_01/probe-bake.json').read_text())
        (self.level/'bake-inputs/overlay.bin').write_bytes(overlay())
        self.save_profile()

    def save_profile(self):
        (self.level/'probe-bake.json').write_text(json.dumps(self.profile))

    def decode(self, success=True):
        result = subprocess.run([str(BAKER),'--decode-water',str(self.level)],capture_output=True,text=True,timeout=10)
        if not success:
            self.assertNotEqual(result.returncode,0,result.stdout)
            return result
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)
        return json.loads(result.stdout)

    def test_novalis_surfaces_and_finished_assets_match_reference(self):
        """Only the new cache fingerprint changes; source geometry and numerical outputs remain exact."""
        expected = water_sources.native_request(water_sources.overlay_memory(overlay()),
                                               self.level/'collision.glb',self.level/'water.bin')
        decoded = self.decode()
        for name in ('surfaces','pool_spacing','boundaries'):
            self.assertEqual(decoded[name],json.loads(json.dumps(expected[name])))
        expected['fingerprint'] = decoded['fingerprint']
        for name, request in (('reference',expected),('native',decoded)):
            path = self.root/(name+'.json')
            path.write_text(json.dumps(request))
            result = subprocess.run([str(BAKER),'--bake-water',str(path),str(self.output/(name+'.bin'))],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)
        for suffix in ('.bin','.mesh'):
            self.assertEqual((self.output/('reference'+suffix)).read_bytes(),(self.output/('native'+suffix)).read_bytes())

    def test_generic_strip_layout_uses_profile_addresses_and_transforms(self):
        """A tiny different image needs no Novalis addresses or game identity."""
        data = bytearray(256)
        struct.pack_into('<II',data,0,192,3)
        for index, point in enumerate(((1,2,4,8),(2,2,4,8),(1,3,4,8))):
            struct.pack_into('<4f',data,128+index*16,*point)
        raw = struct.pack('<4I',64,len(data),0,64)+data
        (self.level/'bake-inputs/overlay.bin').write_bytes(raw)
        config = self.profile['water_sources']
        config['image_bytes'] = 512
        table = copy.deepcopy(config['tables'][1])
        table.update(address=64,count=1,record_stride=8,count_offset=4,source=42,
                     scale=[2,1,1],translation=[-1,0,3])
        config['tables'] = [table]
        self.save_profile()
        body = self.decode()['surfaces'][0]
        self.assertEqual(body['source'],42)
        self.assertEqual(body['triangles'],[[[1,2,9],[3,2,9],[1,3,9]]])
        # A single bounded literal packet exercises the existing shared WAD decompressor.
        payload = bytes((0,len(raw)-18))+raw
        compressed = b'WAD'+struct.pack('<I',16+len(payload))+bytes(9)+payload
        (self.level/'bake-inputs/overlay.bin').write_bytes(compressed)
        self.assertEqual(self.decode()['surfaces'][0],body)

    def test_invalid_profiles_and_pointers_fail_before_output(self):
        """Malformed counts, operations, offsets and unsupported flags cannot be silently accepted."""
        original = copy.deepcopy(self.profile)
        changes = [lambda c:c.update(version=2),lambda c:c.update(format='script'),
                   lambda c:c.update(image_bytes=1<<30),lambda c:c.update(unknown=True),
                   lambda c:c['tables'][0].update(columns=33),
                   lambda c:c['tables'][1].update(pointer_offset=96),
                   lambda c:c['tables'][1].update(vertex_stride=0),
                   lambda c:c['tables'][1].update(position_offsets=[0,4]),
                   lambda c:c['tables'][1].update(kind='unknown'),
                   lambda c:c['tables'][1].update(address=0),
                   lambda c:c['tables'][1].update(count=65)]
        for change in changes:
            with self.subTest(change=changes.index(change)):
                self.profile = copy.deepcopy(original)
                change(self.profile['water_sources'])
                self.save_profile()
                self.decode(success=False)
        self.assertFalse((self.level/'water.bin').exists())

    def test_missing_truncated_and_overlapping_sources_are_rejected(self):
        path = self.level/'bake-inputs/overlay.bin'
        raw = overlay()
        first_size = 16+struct.unpack_from('<I',raw,4)[0]
        for data in (b'bad',raw[:-1],raw[:first_size]+raw[:first_size]):
            with self.subTest(size=len(data)):
                path.write_bytes(data)
                self.decode(success=False)
        path.unlink()
        self.decode(success=False)


if __name__ == '__main__':
    unittest.main()
