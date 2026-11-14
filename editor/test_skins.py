"""skins.py: a moby drawn as another level's or game's model, written beside the packed level."""

import json
import tempfile
import unittest
from pathlib import Path

import skins
from test_library_dock import glb, png


def scene(skin: str, animations: str = "[2, -1]") -> str:
    return ('[gd_scene format=3]\n\n'
            '[ext_resource type="PackedScene" path="res://levels/level_05/mobys/moby_10.tscn" id="1_v"]\n'
            f'[ext_resource type="PackedScene" path="{skin}" id="2_s"]\n\n'
            '[node name="Level" type="Node3D"]\n\n[node name="Game" type="Node3D" parent="."]\n\n'
            '[node name="Mobys" type="Node3D" parent="Game"]\n\n'
            '[node name="Vendor_as_RedShip" parent="Game/Mobys" instance=ExtResource("1_v")]\n'
            'transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 4, 5, 6)\n'
            f'metadata/rc1_skin = "{skin}"\n'
            'metadata/rc1_skin_asset = "rac1/pal/level_03/mobys/75"\n'
            'metadata/rc1_skin_scale = 2.0\n'
            f'metadata/rc1_skin_animations = {animations}\n\n'
            '[node name="Skin" parent="Game/Mobys/Vendor_as_RedShip" instance=ExtResource("2_s")]\n'
            'transform = Transform3D(0.5, 0, 0, 0, 0.5, 0, 0, 0, 0.5, 0, 0, 0)\n\n'
            '[node name="Vendor_0003" parent="Game/Mobys" instance=ExtResource("1_v")]\n'
            'metadata/rc1_index = 3\n')


class Skins(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.project = Path(self.temp.name) / "project"
        level = self.project / "library" / "rac1" / "pal" / "level_03"
        (level / "mobys").mkdir(parents=True)
        (level / "textures").mkdir()
        (level / "mobys" / "moby_75.skin.glb").write_bytes(glb(skinned=True))
        (level / "mobys" / "moby_75.glb").write_bytes(glb(skinned=False))
        (level / "textures" / "t.png").write_bytes(png())
        self.out = Path(self.temp.name) / "out" / "level_05"

    def tearDown(self):
        self.temp.cleanup()

    def test_written_with_its_model_and_textures(self):
        text = scene("res://library/rac1/pal/level_03/mobys/moby_75.skin.glb")
        self.assertEqual([n["name"] for n in skins.skinned(text)], ["Vendor_as_RedShip"])
        changes, problems = skins.write_skins(self.project, text, self.out, {"Vendor_as_RedShip": 300, "Vendor_0003": 3})
        self.assertEqual(problems, [])
        self.assertEqual(changes, ["Vendor_as_RedShip: drawn as rac1/pal/level_03/mobys/75"])
        doc = json.loads((self.out / "skins.json").read_text())
        self.assertEqual(doc["format"], 1)
        [entry] = doc["skins"]
        self.assertEqual((entry["spawn_id"], entry["scale"], entry["animations"]), (300, 2.0, [2, -1]))
        model = self.out / entry["model"]
        self.assertTrue(model.is_file())
        self.assertEqual(len(list((model.parent / "textures").iterdir())), 1)

    def test_godot_packed_array_and_problems(self):
        self.assertEqual(skins.numbers("PackedInt32Array(1, 0)"), [1, 0])
        self.assertEqual(skins.numbers(None), [])
        # A still model, a moby with no spawn id: each drawn as its class, and said so.
        text = scene("res://library/rac1/pal/level_03/mobys/moby_75.glb")
        changes, problems = skins.write_skins(self.project, text, self.out, {"Vendor_as_RedShip": 300})
        self.assertEqual(changes, [])
        self.assertIn("no skeleton or animations", problems[0])
        changes, problems = skins.write_skins(self.project, text, self.out, {})
        self.assertIn("no spawn id", problems[0])
        self.assertFalse((self.out / "skins.json").exists())
        text = scene("res://../outside.glb")
        _, problems = skins.write_skins(self.project, text, self.out, {"Vendor_as_RedShip": 1})
        self.assertIn("outside the project", problems[0])


if __name__ == "__main__":
    unittest.main()
