"""Hand-built collision blocks; no disc data."""

import struct
import unittest

from collision import SCALE, colour, collision_mesh, decode, signed, vertex
from formats import FormatError
from gltf import Gltf
from test_godot import attribute, read_glb


def pack(x, y, z):
    return (x & 0x3ff) | (y & 0x3ff) << 10 | (z & 0xfff) << 20


def leaf(vertices, faces, extra=()):
    """faces: (v0, v1, v2, type); the first len(extra) of them are quads."""
    data = struct.pack("<HBB", len(faces), len(vertices), len(extra))
    data += b"".join(struct.pack("<I", pack(*v)) for v in vertices)
    data += b"".join(struct.pack("<4B", *f) for f in faces) + bytes(extra)
    return data + bytes(-len(data) % 16)


def block(cells, hero=0):
    """cells: {(x, y, z): leaf}; a tree at 0x40 with Z, Y and X nodes for each cell."""
    # Layout: the root, then each slab, each row, then the leaves.
    entries = sorted(cells)
    root = struct.pack("<hH", entries[0][2], entries[-1][2] - entries[0][2] + 1)
    size = 4 + 2 * (entries[-1][2] - entries[0][2] + 1)
    size += -size % 4
    out = bytearray(size)
    pos = size
    slabs = {}
    for k in sorted({c[2] for c in entries}):
        rows = sorted({c[1] for c in entries if c[2] == k})
        slab = bytearray(struct.pack("<hH", rows[0], rows[-1] - rows[0] + 1) + bytes(4 * (rows[-1] - rows[0] + 1)))
        slabs[k] = (pos, rows, slab)
        pos += len(slab)
    rowdata = {}
    for k, (_, rows, slab) in slabs.items():
        for j in rows:
            xs = sorted(c[0] for c in entries if c[1:] == (j, k))
            row = bytearray(struct.pack("<hH", xs[0], xs[-1] - xs[0] + 1) + bytes(4 * (xs[-1] - xs[0] + 1)))
            rowdata[(j, k)] = (pos, xs, row)
            struct.pack_into("<I", slab, 4 + 4 * (j - rows[0]), pos)
            pos += len(row)
    pos += -pos % 16
    body = bytearray()
    for (j, k), (at, xs, row) in rowdata.items():
        for x in xs:
            body_at = pos + len(body)
            struct.pack_into("<I", row, 4 + 4 * (x - xs[0]), body_at << 8 | len(cells[(x, j, k)]) // 16)
            body += cells[(x, j, k)]
    for k, (at, rows, slab) in slabs.items():
        struct.pack_into("<H", out, 4 + 2 * (k - entries[0][2]), at // 4)
    out[:4] = root
    tree = bytes(out) + b"".join(bytes(s[2]) for s in slabs.values())
    tree += b"".join(bytes(r[2]) for r in rowdata.values())
    tree += bytes(-len(tree) % 16) + bytes(body)
    hero_at = 0x40 + len(tree) if hero else 0
    tail = struct.pack("<I", hero) + bytes(12) if hero else b""
    return struct.pack("<II", 0x40, hero_at) + bytes(0x38) + tree + tail


class DecodeTests(unittest.TestCase):
    def test_vertex_fields_and_scales(self):
        self.assertEqual([signed(0x3ff, 10), signed(0x200, 10), signed(0x7ff, 12), signed(0x800, 12)],
                         [-1, -512, 2047, -2048])
        # X and Y step 1/16, Z steps 1/64; the cell (1, -2, 3) is centred at (6, -6, 14).
        centre = tuple((4 * c + 2) * SCALE for c in (1, -2, 3))
        x, y, z = vertex(pack(-3, 17, -129), centre)
        self.assertEqual((x / SCALE, y / SCALE, z / SCALE), (5.8125, -4.9375, 11.984375))

    def test_triangle_quad_and_surface(self):
        square = [(-8, -8, 0), (8, -8, 0), (8, 8, 0), (-8, 8, 0)]
        # A quad record comes first, then a triangle; the quad's fourth index follows.
        data = leaf(square, [(0, 1, 2, 0x0c), (0, 1, 3, 0x2a)], extra=[3])
        result = decode(block({(0, 0, 0): data}))
        self.assertEqual((result.cells, result.faces, len(result.triangles)), (1, 2, 3))
        self.assertEqual(result.surfaces(), {0x0a: 1, 0x0c: 2})
        low = (2 * SCALE) - 8 * 4  # cell centre 2.0 minus 8/16.
        corners = {p for t in result.triangles for p in t[:3]}
        self.assertIn((low, low, 2 * SCALE), corners)

    def test_faces_shared_by_cells_are_merged(self):
        # The same world triangle, seen from the cells (0, 0, 0) and (1, 0, 0).
        a = leaf([(0, 0, 0), (16, 0, 0), (0, 16, 0)], [(0, 1, 2, 8)])
        b = leaf([(-64, 0, 0), (-48, 0, 0), (-64, 16, 0)], [(0, 1, 2, 8)])
        result = decode(block({(0, 0, 0): a, (1, 0, 0): b}))
        self.assertEqual((result.cells, result.faces, len(result.triangles)), (2, 2, 1))

    def test_hero_group_count_and_errors(self):
        a = leaf([(0, 0, 0), (16, 0, 0), (0, 16, 0)], [(0, 1, 2, 8)])
        self.assertEqual(decode(block({(0, 0, 0): a}, hero=5)).hero_groups, 5)
        with self.assertRaises(FormatError):
            decode(block({(0, 0, 0): leaf([(0, 0, 0)], [(0, 1, 2, 8)])}))
        with self.assertRaises(FormatError):
            decode(struct.pack("<II", 0x40, 0x10000) + bytes(0x38))

    def test_front_face_is_counter_clockwise(self):
        # The game's normal is (v2 - v0) x (v1 - v0); here that points down, so glTF's
        # (v1 - v0) x (v2 - v0) must point up.
        a = leaf([(0, 0, 0), (16, 0, 0), (0, 16, 0)], [(0, 2, 1, 8)])
        (p0, p1, p2, _), = decode(block({(0, 0, 0): a})).triangles
        u, v = [[q[i] - p0[i] for i in range(3)] for q in (p1, p2)]
        self.assertGreater(u[0] * v[1] - u[1] * v[0], 0)


class OutputTests(unittest.TestCase):
    def test_colours(self):
        self.assertEqual(colour(0x1f), colour(0x7f))  # Bits 5-7 are not part of the surface.
        self.assertEqual(colour(0x0c), colour(0xac))
        self.assertNotEqual(colour(0x0c), colour(0x08))
        self.assertEqual(len({colour(i) for i in range(32)}), 32)

    def test_glb_has_vertex_colours_and_an_unlit_material(self):
        a = leaf([(0, 0, 0), (16, 0, 0), (0, 16, 0)], [(0, 1, 2, 12)])
        mesh = collision_mesh(decode(block({(0, 0, 0): a})))
        gltf = Gltf()
        gltf.node("Collision", gltf.coloured(mesh, gltf.overlay("collision", 0.5)))
        doc, binary = read_glb(gltf.glb())
        self.assertEqual(doc["extensionsUsed"], ["KHR_materials_unlit"])
        material = doc["materials"][0]
        self.assertEqual((material["alphaMode"], material["doubleSided"]), ("BLEND", True))
        self.assertEqual(material["pbrMetallicRoughness"]["baseColorFactor"][3], 0.5)
        attributes = doc["meshes"][0]["primitives"][0]["attributes"]
        self.assertEqual(len(attribute(doc, binary, attributes["POSITION"])), 3)
        colours = attribute(doc, binary, attributes["COLOR_0"])
        self.assertEqual(len(colours), 3)
        self.assertAlmostEqual(colours[0][3], 1.0)


if __name__ == "__main__":
    unittest.main()
