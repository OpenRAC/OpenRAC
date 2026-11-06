"""Creator destination exports and isolated stacks; synthetic records, no disc data."""
import json
from pathlib import Path
import tempfile
import unittest

import map_mod
import overrides
from test_overrides import glb


def pack(root, name, byte, format=4, level=2):
    root.mkdir(parents=True)
    manifest = {"format": format, "game": "rac1", "version": "pal", "name": name}
    if format in (3, 4):
        manifest["map"] = {"level": level, "spawn": [1, 2, 3]}
    (root / overrides.MANIFEST).write_text(json.dumps(manifest))
    (root / f"level_{level:02}").mkdir()
    (root / f"level_{level:02}/core_pal.bin").write_bytes(bytes([byte]) * 2048)
    (root / 'patches.tsv').write_text(f"{level * 100}\t1\tlevel_{level:02}/core_pal.bin\n")
    return root


class DestinationPacks(unittest.TestCase):
    def test_two_worlds_reuse_one_template_without_overwriting_it_or_each_other(self):
        with tempfile.TemporaryDirectory() as d:
            r = Path(d)
            folder, out = r / 'installed', r / 'stack'
            a, _, _ = overrides.add(pack(r / 'a', 'Harbor', 65), folder)
            b, _, _ = overrides.add(pack(r / 'b', 'Arena', 66), folder)
            overrides.add(pack(r / 'base', 'Campaign edit', 67, format=1, level=3), folder)
            raw = pack(r / 'raw', 'Authoring output', 68, format=1)
            (raw / 'packed.json').write_text('{"version":"pal"}')
            overrides.stack(folder, out, packed=raw)
            registry = json.loads((out / 'destinations.json').read_text())
            self.assertEqual([m['id'] for m in registry['destinations']], sorted([a, b]))
            self.assertFalse((out / 'map.json').exists())
            self.assertFalse((out / 'level_02/core_pal.bin').exists())
            # What a party compares: every world, and the edit they share; the raw output is no layer.
            layers = {layer['id']: layer for layer in json.loads((out / 'stack.json').read_text())['layers']}
            self.assertEqual(len(layers), 3)
            self.assertTrue(all(layer['gameplay'] and len(layer['digest']) == 64 for layer in layers.values()))
            self.assertNotEqual(layers[a]['digest'], layers[b]['digest'])
            self.assertNotIn('200\t', (out / 'patches.tsv').read_text())
            for ident, value in ((a, 65), (b, 66)):
                mount = out / 'destinations' / ident
                self.assertEqual((mount / 'level_02/core_pal.bin').read_bytes(), bytes([value]) * 2048)
                self.assertEqual((mount / 'level_03/core_pal.bin').read_bytes(), bytes([67]) * 2048)
            state = overrides.load_state(folder)
            next(p for p in state['packs'] if p['id'] == b)['enabled'] = False
            overrides.save_state(folder, state)
            overrides.stack(folder, out)
            self.assertFalse((out / 'destinations' / b).exists())
            self.assertEqual(len(json.loads((out / 'destinations.json').read_text())['destinations']), 1)

    def test_standalone_mix_is_rejected_without_replacing_the_previous_stack(self):
        with tempfile.TemporaryDirectory() as d:
            r = Path(d)
            overrides.add(pack(r / 'a', 'Harbor', 65), r / 'installed')
            overrides.stack(r / 'installed', r / 'stack')
            before = (r / 'stack/destinations.json').read_bytes()
            overrides.add(pack(r / 'b', 'Standalone', 66, format=3), r / 'installed')
            with self.assertRaisesRegex(overrides.PackError, 'Standalone maps'):
                overrides.stack(r / 'installed', r / 'stack')
            self.assertEqual((r / 'stack/destinations.json').read_bytes(), before)

    def test_export_is_generic_and_requires_a_return_ship(self):
        with tempfile.TemporaryDirectory() as d:
            r = Path(d)
            packed = pack(r / 'packed', 'Input', 65, format=1)
            (packed / 'packed.json').write_text('{"game":"rac1","version":"pal"}')
            level = r / 'project/levels/level_02'
            level.mkdir(parents=True)
            doc, binary = overrides.glb_parts(glb())
            doc.pop('skins', None)
            for node in doc['nodes']:
                node.pop('skin', None)
            for mesh in doc['meshes']:
                for prim in mesh['primitives']:
                    prim['attributes'].pop('JOINTS_0', None)
                    prim['attributes'].pop('WEIGHTS_0', None)
            (level / 'terrain.glb').write_bytes(overrides.glb_join(doc, binary))
            with self.assertRaisesRegex(overrides.PackError, 'editable level scene'):
                map_mod.export_map(packed, r / 'project', r / 'bad', 'My new world', 2, [4, 5, 6], destination=True)
            (level / 'level_02.tscn').write_text('[node name="Ship" parent="Game/LevelSettings" type="Node3D"]\n'
                'transform = Transform3D(1,0,0,0,1,0,0,0,1,4,5,6)\n')
            result = map_mod.export_map(packed, r / 'project', r / 'export', 'My new world', 2, [4, 5, 6], destination=True)
            self.assertEqual(result['format'], 4)
            self.assertEqual(result['name'], 'My new world')
            self.assertEqual(result['map']['level'], 2) # template stays 2; no manual destination number
            with (level / 'level_02.tscn').open('a') as scene:
                scene.write('[ext_resource type="PackedScene" path="res://mobys/moby_0.tscn" id="hero"]\n'
                            '[node name="Ratchet" parent="Game/Mobys" instance=ExtResource("hero")]\n'
                            'transform = Transform3D(1,0,0,0,1,0,0,0,1,11,12,13)\n')
            update = map_mod.export_map(packed, r / 'project', r / 'named', 'Harbor', 2,
                                       destination=True, map_id='author.harbor')
            self.assertEqual(update['map']['spawn'], [11, 12, 13])
            installed, _, _ = overrides.add(r / 'named', r / 'installed')
            map_mod.export_map(packed, r / 'project', r / 'updated', 'Harbor revised', 2,
                               destination=True, map_id='author.harbor')
            replacement, _, _ = overrides.add(r / 'updated', r / 'installed')
            self.assertEqual(replacement, installed)
            self.assertEqual(len(overrides.load_state(r / 'installed')['packs']), 1)
            result['version'] = 'ntsc'
            with self.assertRaisesRegex(overrides.PackError, 'PAL'):
                overrides.map_settings(result)


if __name__ == '__main__':
    unittest.main()
