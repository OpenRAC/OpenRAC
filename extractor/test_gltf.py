"""The GLB writer (gltf.py): synthetic meshes only."""

import unittest

from glb_check import attribute, read_glb
from gltf import Gltf
from mesh import Mesh


def triangle() -> Mesh:
    mesh = Mesh("tie_7", [(0, 0, 0), (1, 0, 0), (0, 1, 0)], [(0, 0), (1, 0), (0, 1)])
    mesh.add_face(("tie", 3), (0, 1, 2))
    return mesh


class GltfTests(unittest.TestCase):
    def test_layout_attributes_and_shared_texture(self):
        gltf = Gltf()
        material = gltf.material("tie_0003", "../textures/tie_0003.png", cutout=True)
        gltf.node("tie_7", gltf.mesh(triangle(), {("tie", 3): material}))
        doc, binary = read_glb(gltf.glb())
        self.assertFalse([k for k, v in doc.items() if v == []])
        for view in doc["bufferViews"]:
            self.assertEqual(view["byteOffset"] % 4, 0)
        self.assertEqual(doc["images"], [{"uri": "../textures/tie_0003.png"}])
        self.assertEqual((doc["materials"][0]["alphaMode"], doc["materials"][0]["alphaCutoff"]), ("MASK", 0.5))
        primitive = doc["meshes"][0]["primitives"][0]
        self.assertEqual(attribute(doc, binary, primitive["attributes"]["NORMAL"]), [(0, 0, 1)] * 3)
        self.assertEqual(attribute(doc, binary, primitive["attributes"]["TEXCOORD_0"]), [(0, 0), (1, 0), (0, 1)])
        self.assertEqual(doc["accessors"][primitive["attributes"]["POSITION"]]["max"], [1, 1, 0])
        self.assertEqual(doc["scenes"][0]["nodes"], [0])

    def test_materials_are_shared(self):
        gltf = Gltf()
        self.assertEqual(gltf.material("a", "a.png"), gltf.material("a", "a.png"))
        self.assertNotEqual(gltf.material("a", "a.png"), gltf.material("a", "a.png", cutout=True))

    def test_opaque_materials_say_so_in_extras(self):
        gltf = Gltf()
        opaque = gltf.material("a", "a.png", opaque=True)
        self.assertNotEqual(opaque, gltf.material("a", "a.png"))
        self.assertEqual(gltf.doc["materials"][opaque]["extras"], {"opaque": True})
        self.assertNotIn("alphaMode", gltf.doc["materials"][opaque])

    def test_additive_materials_say_so_in_extras(self):
        gltf = Gltf()
        added = gltf.material("a", "a.png", additive=0.5)
        self.assertNotEqual(added, gltf.material("a", "a.png"))
        self.assertEqual(gltf.doc["materials"][added]["extras"], {"additive": 0.5})
        self.assertNotIn("alphaMode", gltf.doc["materials"][added])

    def test_clamped_materials_sample_clamped(self):
        gltf = Gltf()
        doc = gltf.doc
        wraps = []
        for clamp in (0, 1, 2, 3, 1):
            texture = doc["materials"][gltf.material("a", "a.png", clamp=clamp)]["pbrMetallicRoughness"]
            sampler = doc["samplers"][doc["textures"][texture["baseColorTexture"]["index"]]["sampler"]]
            wraps.append((sampler["wrapS"], sampler["wrapT"]))
        self.assertEqual(wraps, [(10497, 10497), (33071, 10497), (10497, 33071), (33071, 33071), (33071, 10497)])
        self.assertEqual(len(doc["samplers"]), 4)


if __name__ == "__main__":
    unittest.main()
