# SPDX-License-Identifier: MIT
# Copyright (c) 2026 the OpenRAC contributors
"""Static donor imports: private dependencies, transforms, optional collision and export freshness."""
import json
import re
from pathlib import Path
import tempfile
import unittest

import collision
import collision_scene
from formats import png
from gltf import Gltf
import gltf_read
import map_mod
from mesh import Mesh
import static_assets


def model(uri):
    g = Gltf()
    mesh = Mesh('decor', [(0, 0, 0), (2, 0, 0), (0, 2, 0)], [(0, 0), (1, 0), (0, 1)],
                normals=[(0, 0, 1)] * 3)
    mesh.add_face(None, (0, 1, 2))
    g.node('decor', g.mesh(mesh, {None: g.material('paint', uri)}))
    return g.glb()


def fixture(root):
    project = root / 'project'
    level = project / 'levels/level_02'
    (level / 'textures').mkdir(parents=True)
    (level / 'collision').mkdir()
    (project / 'project.godot').write_text('config_version=5\n')
    (level / 'textures/shared.png').write_bytes(png(2, 2, 2, bytes([0, 0, 255]) * 4))
    (level / 'terrain.glb').write_bytes(model('textures/shared.png'))
    (level / 'collision/collision.json').write_text(json.dumps({'format': collision_scene.FORMAT, 'tiles': {}, 'hero_walls': {}}))
    scene = level / 'level_02.tscn'
    scene.write_text('[gd_scene format=3]\n[node name="Level" type="Node3D"]\n'
                     '[node name="Game" type="Node3D" parent="."]\n'
                     '[node name="Collision" type="Node3D" parent="Game"]\nmetadata/rc1_collision = true\n')
    donor = root / 'donor/level_01'
    (donor / 'ties').mkdir(parents=True)
    (donor / 'textures').mkdir()
    source = donor / 'ties/tie_1.glb'
    source.write_bytes(model('../textures/shared.png'))
    (donor / 'textures/shared.png').write_bytes(png(2, 2, 2, bytes([255, 0, 0]) * 4))
    return project, level, scene, source


class StaticAssets(unittest.TestCase):
    def test_removing_an_instance_drops_its_geometry_and_unused_texture_from_export(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            project, level, scene, source = fixture(root)
            static_assets.import_asset(source, project, 2, 'Bridge', [10, 20, 30])
            packed = root / 'packed'
            static_assets.write_packed(project, level, scene.read_text(), packed / 'level_02')
            (packed / 'packed.json').write_text('{"game":"rac1","version":"pal"}')
            scene.write_text(re.sub(r'\[node name="Bridge".*?\Z', '', scene.read_text(), flags=re.S))
            stamp = static_assets.write_packed(project, level, scene.read_text(), packed / 'level_02')
            self.assertEqual(stamp['instances'], 0)
            map_mod.export_map(packed, project, root / 'clean', 'A world', 2, [0, 0, 0])
            self.assertEqual(len(list((root / 'clean/level_02/textures').glob('*.png'))), 1)
            self.assertFalse(static_assets.collision_triangles(project, scene.read_text()))

    def test_copy_namespaces_dependencies_and_preserves_sources(self):
        with tempfile.TemporaryDirectory() as tmp:
            project, level, scene, source = fixture(Path(tmp))
            original = source.read_bytes()
            scene.write_text(scene.read_text().replace('[node name="Level"',
                '[sub_resource type="BoxMesh" id="box"]\nsize = Vector3(1,1,1)\n[node name="Level"'))
            before = scene.read_bytes()
            added = static_assets.import_asset(source, project, 2, 'Bridge', [10, 20, 30])
            self.assertEqual(source.read_bytes(), original)
            self.assertEqual(Path(added['backup']).read_bytes(), before)
            self.assertLess(scene.read_text().index('[ext_resource'), scene.read_text().index('[sub_resource'))
            copied = project / added['model'][6:]
            doc = gltf_read.read(copied)
            self.assertTrue(doc.list('images')[0]['uri'].startswith('textures/static_'))
            self.assertEqual(doc.image_bytes(doc.list('images')[0]), source.parent.parent.joinpath('textures/shared.png').read_bytes())
            payload, textures, stamp = static_assets.bake(project, level, scene.read_text())
            self.assertEqual(len(textures), 2) # donor red and target blue shared.png never collide
            self.assertEqual(stamp['instances'], 1)
            self.assertFalse(static_assets.collision_triangles(project, scene.read_text()))
            self.assertTrue(payload.startswith(b'glTF'))
            kept = scene.read_bytes()
            with self.assertRaisesRegex(ValueError, 'unused'):
                static_assets.import_asset(source, project, 2, 'Bridge', [1, 2, 3])
            self.assertEqual(scene.read_bytes(), kept)

    def test_group_transforms_mirroring_and_collision_agree(self):
        with tempfile.TemporaryDirectory() as tmp:
            project, level, scene, source = fixture(Path(tmp))
            static_assets.import_asset(source, project, 2, 'Bridge', [10, 20, 30], True)
            text = scene.read_text().replace('[node name="StaticAssets" type="Node3D" parent="Game"]',
                '[node name="StaticAssets" type="Node3D" parent="Game"]\n'
                'transform = Transform3D(-2,0,0,0,3,0,0,0,1,100,200,0)')
            payload, _, _ = static_assets.bake(project, level, text)
            baked = level / 'baked.glb'
            baked.write_bytes(payload)
            visible = [p for p, _ in collision_scene.gltf_triangles(baked) if all(v[2] == 30 for v in p)]
            self.assertEqual(len(visible), 1)
            a, b, c = visible[0]
            self.assertGreater((b[0]-a[0])*(c[1]-a[1]) - (b[1]-a[1])*(c[0]-a[0]), 0)
            expected = collision.canonical(tuple(collision.quantise(p) for p in visible[0]))
            physical = collision_scene.scene_collision(project, level, text)
            self.assertIn((expected, 0x1f), physical['triangles'])
            self.assertEqual(set(visible[0]), {(80, 260, 30), (76, 260, 30), (80, 266, 30)})

    def test_export_requires_repack_after_moving_or_changing_collision(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            project, level, scene, source = fixture(root)
            static_assets.import_asset(source, project, 2, 'Bridge', [10, 20, 30])
            packed = root / 'packed'
            (packed / 'level_02').mkdir(parents=True)
            (packed / 'packed.json').write_text('{"game":"rac1","version":"pal"}')
            static_assets.write_packed(project, level, scene.read_text(), packed / 'level_02')
            map_mod.export_map(packed, project, root / 'export', 'A world', 2, [0, 0, 0])
            self.assertEqual((root / 'export/level_02/terrain.glb').read_bytes(), (packed / 'level_02/terrain.glb').read_bytes())
            scene.write_text(scene.read_text().replace('rc1_static_collision = false', 'rc1_static_collision = true'))
            with self.assertRaisesRegex(ValueError, 'Pack the project again'):
                map_mod.export_map(packed, project, root / 'stale', 'A world', 2, [0, 0, 0])
            self.assertFalse((root / 'stale').exists())

    def test_missing_texture_or_animated_source_leaves_scene_untouched(self):
        with tempfile.TemporaryDirectory() as tmp:
            project, _, scene, source = fixture(Path(tmp))
            before = scene.read_bytes()
            doc = gltf_read.read(source)
            doc.doc['animations'] = [{'name': 'not static'}]
            g = Gltf()
            g.doc, g.buffer = doc.doc, bytearray(doc.buffers[0])
            source.write_bytes(g.glb())
            with self.assertRaisesRegex(ValueError, 'animations'):
                static_assets.import_asset(source, project, 2, 'Animated', [0, 0, 0])
            source.write_bytes(model('../textures/missing.png'))
            with self.assertRaisesRegex(ValueError, 'not there'):
                static_assets.import_asset(source, project, 2, 'Missing', [0, 0, 0])
            self.assertEqual(scene.read_bytes(), before)


if __name__ == '__main__':
    unittest.main()
