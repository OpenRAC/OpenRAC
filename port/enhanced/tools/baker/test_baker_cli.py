# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Native enhanced-baker CLI smoke checks; run with the executable path, without game data."""
import json
import os
import hashlib
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib

from bake_level import NativeBatch
from native_coordinator import NativeCoordinator
import bake_all

BAKER = Path(sys.argv.pop(1)).resolve() if __name__ == '__main__' and len(sys.argv) > 1 else None


def exported_room(root):
    """A closed box, one lighting boundary and ordinary GLB vertex colours; no retail data."""
    level, output = root/'level', root/'bake'
    level.mkdir()
    output.mkdir()
    quads = [((-8,-8,0),(8,-8,0),(8,8,0),(-8,8,0)),
             ((-8,-8,5),(-8,8,5),(8,8,5),(8,-8,5)),
             ((-8,-8,0),(-8,-8,5),(8,-8,5),(8,-8,0)),
             ((8,8,0),(8,8,5),(-8,8,5),(-8,8,0)),
             ((-8,8,0),(-8,8,5),(-8,-8,5),(-8,-8,0)),
             ((8,-8,0),(8,-8,5),(8,8,5),(8,8,0))]
    positions = [vertex for a,b,c,d in quads for triangle in ((a,b,c),(a,c,d)) for vertex in triangle]
    binary = b''.join(struct.pack('<3f', *vertex) for vertex in positions)
    document = {'asset': {'version':'2.0'}, 'buffers':[{'byteLength':len(binary)}],
                'bufferViews':[{'buffer':0,'byteOffset':0,'byteLength':len(binary)}],
                'accessors':[{'bufferView':0,'componentType':5126,'count':len(positions),'type':'VEC3'}],
                'materials':[{}],
                'meshes':[{'primitives':[{'attributes':{'POSITION':0}}]}], 'nodes':[{'mesh':0}]}
    text = json.dumps(document).encode()
    text += b' '*((-len(text))%4)
    payload = struct.pack('<II',len(text),0x4e4f534a)+text+struct.pack('<II',len(binary),0x004e4942)+binary
    (level/'collision.glb').write_bytes(struct.pack('<4sII',b'glTF',2,12+len(payload))+payload)
    manifest = {'format': 4, 'game': 'rac1', 'version': 'ntsc', 'level': 1,
                'terrain': 'collision.glb', 'placements': 'placements.json',
                'classes': {'tie':{},'shrub':{},'moby':{}}, 'bounds':[[-8,-8,0],[8,8,5]],
                'lights': [[[.4,.5,.3,0],[0,0,-1,0],[0,0,0,0],[0,0,-1,0]] for _ in range(16)]}
    (level/'manifest.json').write_text(json.dumps(manifest))
    (level/'placements.json').write_text('{"ties":[],"shrubs":[],"mobys":[]}')
    gameplay = bytearray(512)
    struct.pack_into('<I', gameplay, 0x80, 0xa0)
    struct.pack_into('<I', gameplay, 0xa0, 1)
    struct.pack_into('<16f', gameplay, 0xc0, 1,0,0,0, 0,.125,0,0, 0,0,.4,0, 0,0,-1,1)
    struct.pack_into('<2I2iI', gameplay, 0xc0+0x40, 0xff282828,0xff282828,0,1,1)
    inputs = level/'bake-inputs'
    inputs.mkdir()
    path = inputs/'gameplay.bin'
    path.write_bytes(gameplay)
    return level, path, output


def emission_mesh(level, authored=False):
    """Two synthetic bright triangles with valid and invalid static light slots, plus one PNG."""
    (level/'textures').mkdir(exist_ok=True)
    def chunk(kind, data):
        return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data))
    png = b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>2I5B',1,1,8,6,0,0,0))
    png += chunk(b'IDAT',zlib.compress(b'\0\xff\xff\xff\xff'))+chunk(b'IEND',b'')
    (level/'textures/panel.png').write_bytes(png)
    positions = [(-2,-2,0),(2,-2,0),(2,2,0),(-2,-2,0),(2,2,0),(-2,2,0)]
    data = b''.join(struct.pack('<3f',*v) for v in positions)
    colours = b''.join(struct.pack('<4f',255/128,255/128,255/128,1) for _ in positions)
    slots = struct.pack('<6f',0,0,0,63,63,63)
    binary = data+colours+slots
    material = {'pbrMetallicRoughness':{'baseColorTexture':{'index':0}}}
    if authored:
        material.update(emissiveFactor=[.5,0,0],extensions={'KHR_materials_emissive_strength':{'emissiveStrength':0}})
    document = {'asset':{'version':'2.0'},'buffers':[{'byteLength':len(binary)}],
        'bufferViews':[{'buffer':0,'byteOffset':0,'byteLength':len(data)},
                       {'buffer':0,'byteOffset':len(data),'byteLength':len(colours)},
                       {'buffer':0,'byteOffset':len(data)+len(colours),'byteLength':len(slots)}],
        'accessors':[{'bufferView':i,'componentType':5126,'count':6,'type':kind}
                     for i,kind in enumerate(('VEC3','VEC4','SCALAR'))],
        'images':[{'uri':'textures/panel.png'}],'textures':[{'source':0}],
        'materials':[material],'meshes':[{'primitives':[{'material':0,
            'attributes':{'POSITION':0,'COLOR_0':1,'_LIGHT_SLOT':2}}]}],'nodes':[{'mesh':0}]}
    text=json.dumps(document).encode(); text+=b' '*((-len(text))%4)
    payload=struct.pack('<II',len(text),0x4e4f534a)+text+struct.pack('<II',len(binary),0x004e4942)+binary
    (level/'panel.glb').write_bytes(struct.pack('<4sII',b'glTF',2,12+len(payload))+payload)


@unittest.skipIf(BAKER is None, 'run with the native enhanced baker path')
class BakerCli(unittest.TestCase):
    def run_baker(self, *arguments):
        return subprocess.run([str(BAKER), *map(str, arguments)], capture_output=True, text=True)

    def test_native_water_fields_and_mesh_match_python_reference(self):
        """Slopes, still pools, absorbing outlets and hydraulic floors retain exact float32 output."""
        sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'water'))
        import bake_water as water
        from test_bake_water import WaterBakeTest
        square=WaterBakeTest().square
        variants=[[(1,square(z=1),(0,0,0))],
                  [(4,[((0,0,1),(4,0,3),(0,4,1)),((0,4,1),(4,0,3),(4,4,3))],(0,0,-1))],
                  [(5,square(z=3),(0,0,0)),(6,square(z=3.2),(0,0,0))]]
        for surfaces in variants:
            with self.subTest(sources=[s[0] for s in surfaces]),tempfile.TemporaryDirectory() as temporary:
                root=Path(temporary);level,_,output=exported_room(root)
                solids,support=water.collision_triangles(level/'collision.glb')
                edges=[{'a':[0,0,1],'b':[0,4,1],'width':1}]
                bodies=[water.bake_body(*surface,open_edges=edges,solids=solids,water_collision=support)
                        for surface in surfaces]
                water.write_bake(output/'reference.bin',bodies,b'x'*32,solids)
                water.write_rest_mesh(output/'reference.mesh',surfaces,fingerprint=b'x'*32)
                request=dict(version=1,collision=str(level/'collision.glb'),fingerprint=(b'x'*32).hex(),
                    pool_spacing=[1,1],boundaries={'open_edges':edges},
                    surfaces=[dict(source=s,pond=s==1,flowing=2<=s<=4,triangles=ts,flow=f) for s,ts,f in surfaces])
                path=root/'input.json';path.write_text(json.dumps(request))
                result=self.run_baker('--bake-water',path,output/'native.bin')
                self.assertEqual(result.returncode,0,result.stdout+result.stderr)
                for suffix in ('.bin','.mesh'):
                    self.assertEqual((output/('reference'+suffix)).read_bytes(),
                                     (output/('native'+suffix)).read_bytes())
                self.assertNotEqual(self.run_baker('--bake-water',path,output/'native.bin').returncode,0)

    def test_native_water_rejects_invalid_requests(self):
        """Invalid coordinates, outlets and unbounded grids cannot publish a water pair."""
        with tempfile.TemporaryDirectory() as temporary:
            root=Path(temporary);level,_,output=exported_room(root)
            request=dict(version=1,collision=str(level/'collision.glb'),fingerprint='a'*64,
                         pool_spacing=[1,1],boundaries={},surfaces=[dict(source=1,pond=True,flowing=False,
                         flow=[0,0,0],triangles=[[[0,0,1],[4,0,1],[0,4,1]]])])
            for change in ({'version':2},{'pool_spacing':[0,1]}, {'fingerprint':'bad'},
                           {'boundaries':{'open_edges':[{'a':[0,0,0],'b':[1,0,0],'width':0}]}},
                           {'surfaces':[{**request['surfaces'][0],'triangles':[[[0,0,1],[99999,0,1],[0,99999,1]]]}]}):
                path=root/'input.json';path.write_text(json.dumps({**request,**change}))
                self.assertNotEqual(self.run_baker('--bake-water',path,output/'invalid.bin').returncode,0)
                self.assertFalse((output/'invalid.bin').exists())
                self.assertFalse((output/'invalid.mesh').exists())

    def test_native_emission_preparation_matches_reference_updates_and_overrides(self):
        """Catalog refreshes, removals and local edits retain the Python profile's meaning."""
        sys.path.insert(0,str(Path(__file__).resolve().parents[4]/'editor'))
        from enhanced.emission import apply_materials
        with tempfile.TemporaryDirectory() as temporary:
            root=Path(temporary); catalog=root/'catalog.json'; defaults=root/'defaults.json'
            levels=[root/'python',root/'native']
            for level in levels:
                (level/'textures').mkdir(parents=True)
                (level/'textures/renamed.png').write_bytes(b'synthetic reviewed lamp')
                (level/'textures/ordinary.png').write_bytes(b'ordinary surface')
            identity=hashlib.sha256(b'synthetic reviewed lamp').hexdigest()
            tracked={'static_mobys':[42],'textures/renamed.png':{'bake_intensity':8}}
            for intensity in (2,3,4,None):
                if intensity == 4:
                    for level in levels:
                        path=level/'probe-emission.json'; value=json.loads(path.read_text())
                        value['textures/renamed.png'].update(runtime_intensity=0,vertex_colour=True)
                        value['points']=[{'position':[1,2,3]}]
                        path.write_text(json.dumps(value))
                materials=[] if intensity is None else [{'name':'fixture lamp','texture_sha256':identity,
                    'emission':{'runtime_intensity':intensity,'diffuse_scale':0}}]
                catalog.write_text(json.dumps({'version':1,'materials':materials}))
                defaults.write_text(json.dumps(tracked))
                apply_materials(levels[0],catalog,tracked)
                result=self.run_baker('--prepare-level-emission',levels[1],catalog,defaults)
                self.assertEqual(result.returncode,0,result.stdout+result.stderr)
                self.assertEqual(json.loads((levels[0]/'probe-emission.json').read_text()),
                                 json.loads((levels[1]/'probe-emission.json').read_text()))
                profile=levels[1]/'probe-emission.json'; before=profile.read_bytes()
                result=self.run_baker('--prepare-level-emission',levels[1],catalog,defaults)
                self.assertEqual(result.returncode,0,result.stdout+result.stderr)
                self.assertIn('emission_profile_changed 0',result.stdout)
                self.assertEqual(profile.read_bytes(),before)
                if intensity == 4:
                    tracked={}

    def test_native_emission_rejects_bad_catalogs_without_replacing_authoring(self):
        """Invalid identities and settings leave the original profile and interrupted staging intact."""
        with tempfile.TemporaryDirectory() as temporary:
            root=Path(temporary); level=root/'level'; level.mkdir()
            path=level/'probe-emission.json'; path.write_text('{"points":[]}')
            stale=level/'probe-emission.json.part'; stale.write_text('interrupted')
            material={'name':'fixture','texture_sha256':'a'*64,'emission':{'runtime_intensity':1}}
            bad=[{}, {'version':1,'materials':[None]},
                 {'version':1,'materials':[material,material]},
                 {'version':1,'materials':[{**material,'texture_sha256':'bad'}]},
                 {'version':1,'materials':[{**material,'emission':{'contrast':0}}]},
                 {'version':1,'materials':[{**material,'emission':{'runtime_intensity':True}}]}]
            for value in bad:
                catalog=root/'catalog.json'; catalog.write_text(json.dumps(value))
                result=self.run_baker('--prepare-level-emission',level,catalog,root/'absent.json')
                self.assertNotEqual(result.returncode,0)
                self.assertEqual(path.read_text(),'{"points":[]}')
                self.assertEqual(stale.read_text(),'interrupted')
                self.assertFalse(list(level.glob('.bake-stage-*')))

    def test_native_emission_audit_matches_reference_static_layers(self):
        """Resident mesh auditing retains terrain clamps, per-instance light slots and explicit definitions."""
        from emission_audit import audit
        for authored,override in ((False,False),(False,True),(True,False)):
            with self.subTest(authored=authored,override=override),tempfile.TemporaryDirectory() as temporary:
                root=Path(temporary);level,_,output=exported_room(root)
                emission_mesh(level,authored)
                manifest=json.loads((level/'manifest.json').read_text());manifest['terrain']='panel.glb'
                manifest['classes'].update(tie={'1':{'mesh':'panel.glb'}},shrub={'1':{'mesh':'panel.glb'}})
                (level/'manifest.json').write_text(json.dumps(manifest))
                identity=[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1]
                placements={'ties':[{'class':1,'matrix':identity,'colours':[0xfff3f3f3]*64},
                                    {'class':1,'matrix':identity,'colours':[0]*64}],
                            'shrubs':[{'class':1,'matrix':identity,'colours':[0xfff3f3f3]*24}], 'mobys':[]}
                (level/'placements.json').write_text(json.dumps(placements))
                if override:
                    (level/'probe-emission.json').write_text('{"textures/panel.png":{"runtime_intensity":0}}')
                reference=root/'reference';reference.mkdir();expected=audit(level,reference)
                with NativeBatch([str(BAKER),'--level-dir',str(level),'--batch-output',str(output)],
                                 output/'batch.log'):
                    actual=json.loads((output/'emission-audit.json').read_text())
                self.assertEqual(actual,expected)

    def test_native_coverage_matches_reference_floors_walls_and_overhangs(self):
        """The native two-sided flood preserves the reference's coverage and diagnostic counters."""
        from test_surface_coverage import room, SurfaceCoverage
        from surface_coverage import extend_regions
        geometry_cases = [room(), room(walls=False), room()[4:6], room()+room(floor=12, roof=32),
                          room()+room(64,32,96,64), [list(reversed(t)) for t in room()]]
        coarse = SurfaceCoverage.coarse
        region = SurfaceCoverage.region
        for triangles in geometry_cases:
            with self.subTest(triangles=len(triangles)), tempfile.TemporaryDirectory() as directory:
                level, _, output = exported_room(Path(directory))
                # Reuse the synthetic export's layout with this test's collision triangle soup.
                path = level/'collision.glb'
                old = path.read_bytes()
                size = struct.unpack_from('<I', old, 12)[0]
                document = json.loads(old[20:20+size])
                positions = [p for triangle in triangles for p in triangle]
                binary = b''.join(struct.pack('<3f', *point) for point in positions)
                document['buffers'][0]['byteLength'] = len(binary)
                document['bufferViews'][0]['byteLength'] = len(binary)
                document['accessors'][0]['count'] = len(positions)
                text = json.dumps(document).encode()
                text += b' '*((-len(text))%4)
                payload = struct.pack('<II',len(text),0x4e4f534a)+text+struct.pack('<II',len(binary),0x004e4942)+binary
                path.write_bytes(struct.pack('<4sII',b'glTF',2,12+len(payload))+payload)
                command = [str(BAKER),'--level-dir',str(level),'--batch-output',str(output)]
                for anchored in (False, True):
                    seed = {**region, 'bounds': [40,40,-4,88,56,8],
                            'source_transitions': [] if anchored else [2]}
                    markers = [{'index':1, 'position':[48,48,0], 'ground_z':0}]
                    expected = extend_regions([seed], triangles, coarse, [4]*3, markers=markers)
                    with NativeBatch(command, output/f'coverage-{anchored}.log') as native:
                        actual = extend_regions([seed], triangles, coarse, [4]*3, markers=markers,
                                                native_query=native.coverage)
                        self.assertEqual(actual, expected)

    def test_output_pipeline_preserves_bytes_order_and_resume(self):
        """Local output jobs keep serial bytes, ordered verification and the existing resume path."""
        assets = []
        for threads in (1, 3):
            with self.subTest(threads=threads), tempfile.TemporaryDirectory() as directory:
                level, _, output = exported_room(Path(directory))
                command = [str(BAKER), '--level-dir', str(level), '--batch-output', str(output),
                           '--spacing', '16', '--rays', '16', '--bounces', '0', '--threads', str(threads),
                           '--resume']
                with NativeBatch(command, output/'batch.log') as native:
                    identity = next(line.split()[1] for line in native.inspection.splitlines()
                                    if line.startswith('scene_hash '))
                    plan = {'version':1, 'scene_hash':identity,
                            'adaptive_density':{'enabled':True, 'relative_error':.1, 'absolute_error':.02},
                            'regions':[
                        {'bounds':[-4,-4,0,4,4,4], 'spacing':[4]*3, 'max_distance':12},
                        {'bounds':[0,-4,0,8,4,4], 'spacing':[4]*3, 'max_distance':12}]}
                    native.finish(plan)
                names = ['sparse.orpg', 'region-000.orpg', 'region-001.orpg']
                verified = [line.split()[1] for line in (output/'batch.log').read_text().splitlines()
                            if line.startswith('Verified ')]
                self.assertEqual(verified, names)
                before = {name:(output/name).read_bytes() for name in names}
                assets.append(before)
                for name in names:
                    self.assertIn('verified_runtime_rounding 1', (output/(name+'.txt')).read_text())
                with NativeBatch(command+['--resume'], output/'resume.log') as native:
                    native.finish(plan)
                self.assertEqual(before, {name:(output/name).read_bytes() for name in names})
                kept = [line.split()[1] for line in (output/'resume.log').read_text().splitlines()
                        if line.startswith('Kept ')]
                self.assertEqual(kept, names)
        self.assertEqual(assets[0], assets[1])

    def test_native_final_assembly_matches_reference_plans_files_and_resume(self):
        """Native assembly retains bounds, distance caps, reports and verified grid bytes."""
        import bake_level
        import math
        from dataclasses import asdict
        for changes in ({'coarse_padding':2}, {'coarse_bounds':'scene'}, {'enabled':False}, {'no_gameplay':True}):
            with self.subTest(changes=changes), tempfile.TemporaryDirectory() as temporary:
                level, gameplay, output = exported_room(Path(temporary))
                has_gameplay = not changes.get('no_gameplay',False)
                settings = bake_level.RegionSettings(**{k:v for k,v in changes.items() if k != 'no_gameplay'})
                profile = {'dense_spacing':[4,4,2], 'probe_format_version':4,'bake_version':1}
                reports, assets = [], []
                for mode in ('reference', 'native'):
                    directory = output/mode
                    directory.mkdir()
                    command = [str(BAKER),'--level-dir',str(level),'--batch-output',str(directory),
                               '--rays','16','--bounces','0','--threads','1','--resume']
                    with NativeBatch(command,directory/'batch.log') as native:
                        coarse, emission, identity = bake_level.inspection(native.inspection)
                        request = native.planning_request(level,coarse,profile,gameplay.read_bytes() if has_gameplay else None,settings.planner_options())
                        request.update(type='bake_level',region_settings=asdict(settings),scene_hash=identity,emission=emission)
                        if mode == 'reference':
                            plan = native.plan(level,coarse,profile,gameplay.read_bytes(),settings.planner_options()) \
                                if settings.enabled and has_gameplay else dict(regions=[],note=(
                                    'No gameplay to seed dense regions from (a scene, not a level of the disc).' if settings.enabled
                                    else 'Dense regions disabled by the per-level profile.'))
                            plan.update(region_settings=asdict(settings),bake_profile=profile,scene_hash=identity,emission=emission)
                            bake_level.write_json(directory/'seed-plan.json',plan)
                            regions = [dict(bounds=r['bounds'],spacing=r['spacing'],lattice_bounds=r['lattice_bounds'],
                                max_distance=1.5*math.sqrt(sum(v*v for v in r['spacing']))) for r in plan['regions']]
                            transport = dict(version=1,scene_hash=identity,regions=regions,
                                adaptive_density=dict(enabled=False,relative_error=.1,absolute_error=.02))
                            if settings.coarse_bounds == 'local_regions' and regions:
                                transport['coarse_bounds'] = [min(r['bounds'][a] for r in regions)-settings.coarse_padding for a in range(3)] \
                                    + [max(r['bounds'][a+3] for r in regions)+settings.coarse_padding for a in range(3)]
                            native.finish(transport)
                        else:
                            native.finish(request)
                    report = json.loads((directory/'seed-plan.json').read_text())
                    reports.append(report)
                    names = ['sparse.orpg']+[f'region-{i:03d}.orpg' for i in range(len(report['regions']))]
                    assets.append({name:(directory/name).read_bytes() for name in names})
                    if mode == 'native':
                        registry = dict(regions=names[1:])
                        for filename in ('regions.json','sparse.orpg.regions.json'):
                            self.assertEqual(json.loads((directory/filename).read_text()),registry)
                        result = json.loads((directory/'bake-result.json').read_text())
                        self.assertEqual(result,dict(version=1,game_version=coarse['game_version'],level=coarse['level'],
                            scene_hash=identity,sparse='sparse.orpg',**registry,verified=True,emission=emission,
                            probe_format_version=4,bake_version=1))
                        with NativeBatch(command,directory/'resume.log') as native:
                            native.finish(request)
                        self.assertEqual(assets[-1],{name:(directory/name).read_bytes() for name in names})
                        self.assertEqual(sum(line.startswith('Kept ') for line in (directory/'resume.log').read_text().splitlines()),len(names))
                        # A changed report invalidates stale grid-owned files, while private files survive.
                        request['region_settings']['coarse_padding'] += 1
                        (directory/'region-999.orpg.txt').write_text('old grid report')
                        (directory/'keep.txt').write_text('private')
                        (directory/'my-copy.orpg.notes').write_text('private probe copy')
                        with NativeBatch(command,directory/'changed.log') as native:
                            native.finish(request)
                        self.assertFalse((directory/'region-999.orpg.txt').exists())
                        self.assertEqual((directory/'keep.txt').read_text(),'private')
                        self.assertEqual((directory/'my-copy.orpg.notes').read_text(),'private probe copy')
                        self.assertEqual(sum(line.startswith('Baking ') for line in (directory/'changed.log').read_text().splitlines()),len(names))
                self.assertEqual(reports[0],reports[1])
                self.assertEqual(assets[0],assets[1])

    def test_failed_native_registration_does_not_report_completion(self):
        """A registry write failure leaves traced data available but never marks the bake verified."""
        import bake_level
        from dataclasses import asdict
        with tempfile.TemporaryDirectory() as temporary:
            level, _, output = exported_room(Path(temporary))
            (output/'regions.json').mkdir()
            command = [str(BAKER),'--level-dir',str(level),'--batch-output',str(output),
                       '--rays','16','--bounces','0','--resume']
            with NativeBatch(command,output/'batch.log') as native:
                coarse, emission, identity = bake_level.inspection(native.inspection)
                settings = bake_level.RegionSettings(enabled=False)
                request = native.planning_request(level,coarse,dict(probe_format_version=4,bake_version=1),None,settings.planner_options())
                request.update(type='bake_level',region_settings=asdict(settings),scene_hash=identity,emission=emission)
                with self.assertRaises(bake_level.diagnostics.NativeFailure):
                    native.finish(request)
            self.assertTrue((output/'sparse.orpg').is_file())
            self.assertFalse((output/'bake-result.json').exists())
            self.assertNotIn('Probe batch complete',(output/'batch.log').read_text())

    def test_native_final_assembly_rejects_stale_identity_before_removing_resume_files(self):
        """A rejected automatic request preserves prior grid bytes and unrelated private files."""
        import bake_level
        from dataclasses import asdict
        with tempfile.TemporaryDirectory() as temporary:
            level, gameplay, output = exported_room(Path(temporary))
            (output/'sparse.orpg').write_bytes(b'previous grid')
            (output/'seed-plan.json').write_text('{"regions":[]}')
            (output/'keep.txt').write_text('private')
            command = [str(BAKER),'--level-dir',str(level),'--batch-output',str(output),'--resume']
            with NativeBatch(command,output/'batch.log') as native:
                coarse,emission,identity = bake_level.inspection(native.inspection)
                settings = bake_level.RegionSettings(enabled=False)
                request = native.planning_request(level,coarse,{},None,settings.planner_options())
                request.update(type='bake_level',region_settings=asdict(settings),scene_hash=identity+'stale',emission=emission)
                with self.assertRaises(bake_level.diagnostics.NativeFailure):native.finish(request)
            self.assertEqual((output/'sparse.orpg').read_bytes(),b'previous grid')
            self.assertEqual((output/'keep.txt').read_text(),'private')
            self.assertFalse((output/'bake-result.json').exists())

    def test_native_planning_matches_python_seeds_coverage_and_partitioning(self):
        """Complete native reports retain deterministic boxes, source evidence and UTF-8 labels."""
        import bake_level
        import seed_regions
        for overrides in ({}, {'max_probes':64}, {'no_geometry_coverage':True}, {'no_moby_seeds':True}):
            with self.subTest(options=overrides), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                level, gameplay, output = exported_room(root)
                placements = {'mobys':[{'index':i, 'class':500,
                    'matrix':[1,0,0,0,0,1,0,0,0,0,1,0,x,y,z,1]} for i,(x,y,z) in enumerate(
                        ((-4,-4,1),(4,4,1),(0,0,20)))]}
                (level/'placements.json').write_text(json.dumps(placements))
                whitelist = root/'whitelist.json'
                whitelist.write_text(json.dumps({'version':1,'game':'rac1','classes':[
                    {'class':500,'name':'mushroom\U0001f344'*3000,'radius':3,'above':3}]}))
                options = bake_level.RegionSettings(**overrides).planner_options()
                options.moby_whitelist = whitelist
                profile = {'dense_spacing':[4,4,4], 'resolution':2}
                plans = []
                for mode in ('python','native'):
                    directory = output/mode
                    directory.mkdir()
                    with NativeBatch([str(BAKER),'--level-dir',str(level),'--batch-output',str(directory)],
                                     directory/'batch.log') as batch:
                        coarse,_,_ = bake_level.inspection(batch.inspection)
                        if mode == 'python':
                            plan = seed_regions.plan_level(level,coarse,profile,gameplay.read_bytes(),options,
                                                           coverage_query=batch.coverage)
                        else:
                            plan = batch.plan(level,coarse,profile,gameplay.read_bytes(),options)
                        plans.append(plan)
                self.assertEqual(plans[0], plans[1])

    def test_coordinator_runs_native_region_planning_before_the_final_grid_plan(self):
        """The automatic coordinator accepts one native planning request and then the final bake plan."""
        import bake_level
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            level, gameplay, output = exported_room(root)
            plans = []
            with NativeCoordinator(BAKER,root,threads=2,jobs=1,memory=1) as session:
                def bake(index):
                    command = [str(BAKER),'--level-dir',str(level),'--batch-output',str(output),
                               '--rays','16','--bounces','0','--threads','2']
                    with NativeBatch(command,output/'batch.log',process_factory=session.process_factory(index)) as batch:
                        coarse,_,identity = bake_level.inspection(batch.inspection)
                        plans.append(batch.plan(level,coarse,{'dense_spacing':[4,4,4]},gameplay.read_bytes(),
                                                bake_level.RegionSettings().planner_options()))
                        batch.finish({'version':1,'scene_hash':identity,'regions':[]})
                session.run([dict(id=0,weight=1,memory=1)],bake)
            self.assertTrue(plans[0]['regions'])
            self.assertTrue((output/'sparse.orpg').is_file())

    def test_native_region_planning_rejects_invalid_controls(self):
        """Malformed controls stop native planning without publishing grid files."""
        import bake_level
        for changes in ({'max_probes':63}, {'spacing':0}, {'vertical_spacing':0},
                        {'max_regions':0}, {'no_moby_seeds':'yes'}):
            with self.subTest(changes=changes), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                level, gameplay, output = exported_room(root)
                options = bake_level.RegionSettings().planner_options()
                for key, value in changes.items():
                    setattr(options, key, value)
                with NativeBatch([str(BAKER),'--level-dir',str(level),'--batch-output',str(output)],
                                 output/'batch.log') as batch:
                    coarse,_,_ = bake_level.inspection(batch.inspection)
                    with self.assertRaises(bake_level.diagnostics.NativeFailure):
                        batch.plan(level, coarse, {'dense_spacing':[4,4,4]},
                                   gameplay.read_bytes(), options)
                self.assertEqual(list(output.glob('*.orpg')), [])

    def test_profiles_and_help_use_one_executable(self):
        result = self.run_baker('--print-interior-profile')
        self.assertEqual(result.returncode, 0, result.stderr)
        profile = json.loads(result.stdout)
        self.assertEqual(profile['revision'], 1)
        self.assertEqual(profile['spacing'], 2)
        help_text = self.run_baker('--help')
        self.assertEqual(help_text.returncode, 0, help_text.stderr)
        self.assertIn('--infer-interiors', help_text.stdout)
        self.assertIn('--batch-output', help_text.stdout)

    def test_interior_report_supports_both_regions_and_preserves_existing_output(self):
        with tempfile.TemporaryDirectory(prefix='enhanced baker ') as temporary:
            root = Path(temporary)
            source, output = root/'input.json', root/'output.json'
            for version in ('rac1-pal', 'rac1-ntsc'):
                data = {'format': 1, 'game': version, 'level': 1, 'source_sha256': 'a'*64,
                        'triangles': [[0, 0, 0, 8, 0, 0, 0, 8, 0, 1],
                                      [8, 0, 0, 8, 8, 0, 0, 8, 0, 1]], 'transitions': []}
                source.write_text(json.dumps(data))
                result = self.run_baker('--infer-interiors', source, output)
                self.assertEqual(result.returncode, 0, result.stderr)
                report = json.loads(output.read_text())
                self.assertEqual(report['game'], version)
                self.assertEqual(report['source_sha256'], data['source_sha256'])
                self.assertTrue(report['cells'])
                previous = output.read_bytes()
                result = self.run_baker('--infer-interiors', source, output)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(output.read_bytes(), previous)
                output.unlink()

    def test_invalid_stage_arguments_and_missing_inputs_fail(self):
        self.assertNotEqual(self.run_baker('--infer-interiors').returncode, 0)
        self.assertNotEqual(self.run_baker('--infer-interiors', 'missing.json', 'unused.json').returncode, 0)

    def test_native_preparation_generates_sources_and_reuses_its_cache(self):
        with tempfile.TemporaryDirectory(prefix='native interiors ') as temporary:
            level, gameplay, output = exported_room(Path(temporary))
            arguments = ('--prepare-level-interiors', level, gameplay, output)
            result = self.run_baker(*arguments)
            self.assertEqual(result.returncode, 0, result.stderr)
            plan = json.loads((level/'probe-interiors.json').read_text())
            self.assertTrue(plan['samples'])
            self.assertEqual(plan['source_sha256'], hashlib.sha256(gameplay.read_bytes()+(level/'collision.glb').read_bytes()).hexdigest())
            report = json.loads((output/'interior-inference.json').read_text())
            self.assertEqual(report['generation_sha256'], plan['generation_sha256'])
            files = [level/'probe-interiors.json', output/'interior-inference.json', output/'interior-input.json']
            previous = [path.stat().st_mtime_ns for path in files]
            result = self.run_baker(*arguments)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual([path.stat().st_mtime_ns for path in files], previous)
            (output/'interior-input.json').unlink()
            result = self.run_baker(*arguments)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertTrue((output/'interior-input.json').is_file())

    def test_native_preparation_keeps_explicit_sources_and_failed_bakes_keep_old_outputs(self):
        with tempfile.TemporaryDirectory(prefix='native explicit ') as temporary:
            level, gameplay, output = exported_room(Path(temporary))
            explicit = '{"version":1,"level":1,"settings":{},"samples":[]}'
            (level/'probe-interiors.json').write_text(explicit)
            result = self.run_baker('--prepare-level-interiors', level, gameplay, output)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual((level/'probe-interiors.json').read_text(), explicit)
            report = (output/'interior-inference.json').read_bytes()
            gameplay.write_bytes(b'truncated')
            result = self.run_baker('--prepare-level-interiors', level, gameplay, output)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual((level/'probe-interiors.json').read_text(), explicit)
            self.assertEqual((output/'interior-inference.json').read_bytes(), report)
            self.assertFalse(list(level.glob('.bake-stage-*')))
            self.assertFalse(list(output.glob('.bake-stage-*')))

    def test_persistent_batch_prepares_interiors_then_bakes_gi_in_one_worker(self):
        with tempfile.TemporaryDirectory(prefix='persistent interiors ') as temporary:
            level, gameplay, output = exported_room(Path(temporary))
            command = [str(BAKER), '--level-dir', str(level), '--batch-output', str(output),
                       '--prepare-interiors', str(gameplay), '--spacing', '16', '--rays', '16', '--bounces', '1']
            with subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                  stderr=subprocess.STDOUT, text=True) as process:
                before = []
                for line in process.stdout:
                    before.append(line)
                    if line.strip() == 'Probe batch ready':
                        break
                self.assertTrue(before and before[-1].strip() == 'Probe batch ready', ''.join(before))
                self.assertTrue((level/'probe-interiors.json').is_file())
                scene_hash = next(line.split()[1] for line in before if line.startswith('scene_hash '))
                process.stdin.write(json.dumps({'version':1,'scene_hash':scene_hash,'regions':[]})+'\n')
                process.stdin.flush()
                process.stdin.close()
                remaining = process.stdout.read()
                self.assertEqual(process.wait(), 0, ''.join(before)+remaining)
                self.assertIn('Probe batch complete', remaining)
            self.assertTrue((output/'sparse.orpg').is_file())

    def test_batch_rejects_same_size_source_edit_with_unchanged_mtime(self):
        """Shared identities must not replace the content check immediately before tracing."""
        with tempfile.TemporaryDirectory(prefix='changed bake source ') as temporary:
            level, _, output = exported_room(Path(temporary))
            command = [str(BAKER), '--level-dir', str(level), '--batch-output', str(output),
                       '--spacing', '16', '--rays', '16', '--bounces', '0']
            with subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                  stderr=subprocess.STDOUT, text=True) as process:
                before = []
                for line in process.stdout:
                    before.append(line)
                    if line.strip() == 'Probe batch ready':
                        break
                self.assertTrue(before and before[-1].strip() == 'Probe batch ready', ''.join(before))
                identity = next(line.split()[1] for line in before if line.startswith('scene_hash '))
                source = level/'manifest.json'
                old = source.stat()
                data = source.read_bytes()
                # JSON whitespace stays valid, with identical size and timestamp.
                self.assertIn(b' ', data)
                source.write_bytes(data.replace(b' ', b'\t', 1))
                os.utime(source, ns=(old.st_atime_ns, old.st_mtime_ns))
                process.stdin.write(json.dumps({'version':1,'scene_hash':identity,'regions':[]})+'\n')
                process.stdin.flush()
                process.stdin.close()
                remaining = process.stdout.read()
                self.assertNotEqual(process.wait(), 0, ''.join(before)+remaining)
                self.assertIn('source level changed during planning', remaining)
            self.assertFalse((output/'sparse.orpg').exists())

    def coordinated_fixture(self, root, count=2):
        fixtures = []
        for index in range(count):
            parent = root/str(index)
            parent.mkdir()
            fixtures.append(exported_room(parent))
        return fixtures

    def coordinated_bake(self, session, index, fixture):
        level, _, output = fixture
        command = [str(BAKER), '--level-dir', str(level), '--batch-output', str(output),
                   '--spacing', '16', '--rays', '16', '--bounces', '1']
        with NativeBatch(command, output/'batch.log', process_factory=session.process_factory(index)) as native:
            identity = next(line.split()[1] for line in native.inspection.splitlines()
                            if line.startswith('scene_hash '))
            coverage = native.coverage(dict(lower=[-16,-16,-16], upper=[16,16,16], spacing=[2,2,2],
                max_rise=32, padding=3, seeds=[dict(bounds=[-4,-4,-2,4,4,4], anchors=[], require_anchor=False)]))
            self.assertGreater(coverage[0]['indoor'], 0)
            native.finish({'version': 1, 'scene_hash': identity, 'regions': []})
        return (output/'sparse.orpg').read_bytes()

    def test_coordinator_memory_admission_prioritises_large_levels_and_releases_reservations(self):
        with tempfile.TemporaryDirectory(prefix='native scheduler ') as temporary:
            root = Path(temporary)
            fixtures = self.coordinated_fixture(root)
            order, assets = [], {}
            with NativeCoordinator(BAKER, root, threads=2, jobs=2, memory=1) as session:
                def bake(index):
                    order.append(index)
                    assets[index] = self.coordinated_bake(session, index, fixtures[index])
                session.run([dict(id=0, weight=1, memory=1), dict(id=1, weight=2, memory=1)], bake)
            self.assertEqual(order, [1, 0])
            self.assertEqual(assets[0], assets[1])

    def test_coordinator_shares_workers_between_concurrent_levels_with_identical_output(self):
        for threads in (1, 2):
            with self.subTest(threads=threads), tempfile.TemporaryDirectory(prefix='shared scheduler ') as temporary:
                root = Path(temporary)
                fixtures = self.coordinated_fixture(root)
                assets = {}
                with NativeCoordinator(BAKER, root, threads=threads, jobs=2, memory=2) as session:
                    def bake(index):
                        assets[index] = self.coordinated_bake(session, index, fixtures[index])
                    session.run([dict(id=index, weight=1, memory=1) for index in range(2)], bake)
                self.assertEqual(assets[0], assets[1])

    def test_coordinator_rejects_oversized_reservations_and_disconnects_without_hanging(self):
        schedule = dict(version=1, threads=1, jobs=1, memory=1,
                        levels=[dict(id=0, weight=1, memory=2)])
        result = subprocess.run([str(BAKER), '--coordinate-levels'], input=json.dumps(schedule)+'\n',
                                capture_output=True, text=True, timeout=15)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('memory budget', result.stdout)
        schedule['levels'][0]['memory'] = 1
        result = subprocess.run([str(BAKER), '--coordinate-levels'], input=json.dumps(schedule)+'\n',
                                capture_output=True, text=True, timeout=15)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('input closed', result.stdout)

    def test_failed_python_preparation_releases_memory_for_the_next_level(self):
        with tempfile.TemporaryDirectory(prefix='scheduler recovery ') as temporary:
            root = Path(temporary)
            fixtures = self.coordinated_fixture(root)
            completed = []
            with NativeCoordinator(BAKER, root, threads=2, jobs=2, memory=1) as session:
                def bake(index):
                    if index == 0:
                        raise RuntimeError('preparation failed')
                    self.coordinated_bake(session, index, fixtures[index])
                    completed.append(index)
                with self.assertRaisesRegex(RuntimeError, 'preparation failed'):
                    session.run([dict(id=0, weight=2, memory=1), dict(id=1, weight=1, memory=1)], bake)
            self.assertEqual(completed, [1])

    def test_automatic_flow_publishes_both_levels_and_reuses_verified_cache(self):
        with tempfile.TemporaryDirectory(prefix='launcher coordinator ') as temporary:
            root = Path(temporary)
            fixtures = self.coordinated_fixture(root)
            exports = root/'exports'
            exports.mkdir()
            for index, (level, _, _) in enumerate(fixtures):
                level.rename(exports/f'level_{index:02}')
                manifest_path = exports/f'level_{index:02}'/'manifest.json'
                manifest = json.loads(manifest_path.read_text())
                manifest['level'] = 2  # No level-specific liquid inputs are needed by this box fixture.
                manifest_path.write_text(json.dumps(manifest))
                (exports/f'level_{index:02}'/'probe-bake.json').write_text(json.dumps({
                    'version': 1, 'regions': {'enabled': False, 'interior_lighting': False}}))
            cache = root/'cache'
            options = ['--rays', '16', '--bounces', '1', '--threads', '2']
            self.assertTrue(bake_all.bake_all(exports, cache, BAKER, update=True, jobs=2, options=options))
            first = [(cache/f'level_{index:02}'/'sparse.orpg').stat().st_mtime_ns for index in range(2)]
            self.assertTrue(bake_all.bake_all(exports, cache, BAKER, update=True, jobs=2, options=options))
            self.assertEqual(first, [(cache/f'level_{index:02}'/'sparse.orpg').stat().st_mtime_ns for index in range(2)])
            self.assertTrue(all(record['verified'] for record in json.loads((cache/'bake-all-result.json').read_text())['levels']))


if __name__ == '__main__':
    unittest.main()
