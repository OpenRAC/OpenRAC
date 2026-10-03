"""Hand-built skies; no disc data."""

import math
import struct
import unittest

from formats import FormatError, Texture
from mesh import Mesh
from sky import Sky, panorama, sky


def fixture(textured=True):
    """Header at 0, texture def at 0x50, shell at 0x80, cluster at 0xa0, cluster data at 0xc0."""
    data = bytearray(0x542)
    struct.pack_into("<h", data, 6, 1)                        # One shell.
    struct.pack_into("<h", data, 0xc, 1)                      # One texture.
    struct.pack_into("<II", data, 0x10, 0x50, 0x140)          # Texture defs and data.
    struct.pack_into("<I", data, 0x20, 0x80)                  # Shell 0.
    struct.pack_into("<4I", data, 0x50, 0, 1024, 2, 1)        # Palette at +0, pixels at +1024, 2x1.
    struct.pack_into("<II", data, 0x80, 1, 0 if textured else 1)
    struct.pack_into("<I6H", data, 0xa0, 0xc0, 3, 1, 0, 24, 36, 40)
    struct.pack_into("<12h", data, 0xc0, 0, 0, 0, 128, 1024, 0, 0, 64, 0, 1024, 0, 0)
    if textured:
        struct.pack_into("<6h", data, 0xd8, 0, 0, 4096, 0, 0, 4096)
    else:
        data[0xd8:0xe4] = bytes((255, 0, 0, 128, 0, 255, 0, 64, 0, 0, 255, 0))
    data[0xe4:0xe8] = bytes((0, 1, 2, 0 if textured else 255))
    data[0x140:0x148] = bytes((255, 0, 0, 128, 0, 255, 0, 0))
    data[0x540:0x542] = bytes((0, 1))
    return data


def octahedron(name, colour, texture=None):
    """A closed shell around the camera: eight faces, all one colour."""
    points = [(1, 0, 0), (0, 1, 0), (-1, 0, 0), (0, -1, 0), (0, 0, 1), (0, 0, -1)]
    mesh = Mesh(name, points, [(0.5, 0.5)] * 6, [colour] * 6)
    for i in range(4):
        mesh.add_face(texture, (i, (i + 1) % 4, 4))
        mesh.add_face(texture, ((i + 1) % 4, i, 5))
    return mesh


def pixel(image, width, x, y):
    return tuple(image[(y * width + x) * 3:(y * width + x) * 3 + 3])


class SkyTests(unittest.TestCase):
    def test_textured_shell_positions_uvs_alpha_and_reversed_winding(self):
        result = sky(fixture())
        shell, = result.shells
        self.assertEqual(shell.positions, [(0, 0, 0), (1, 0, 0), (0, 1, 0)])
        self.assertEqual(shell.uvs, [(0, 0), (1, 0), (0, 1)])
        self.assertEqual([c[3] for c in shell.colours], [1, 0.5, 0])
        self.assertEqual(shell.faces, {("sky", 0): [(2, 1, 0)]})
        self.assertEqual(result.textures[0].colours[1][:2], (0, 255))

    def test_untextured_shell_keeps_vertex_colours(self):
        shell, = sky(fixture(textured=False)).shells
        self.assertEqual(shell.faces, {None: [(2, 1, 0)]})
        self.assertEqual(shell.colours, [(1, 0, 0, 1), (0, 1, 0, 0.5), (0, 0, 1, 0)])

    def test_rejects_bad_indices_textures_and_alpha(self):
        for offset, value in ((0xe4, 3), (0xe7, 1), (0xc6, 0x81)):
            data = fixture()
            data[offset] = value
            with self.subTest(offset=offset), self.assertRaises(FormatError):
                sky(data)
        data = fixture(textured=False)
        data[0xe7] = 0  # A textured face on an untextured shell.
        with self.assertRaises(FormatError):
            sky(data)


class PanoramaTests(unittest.TestCase):
    def test_shells_blend_in_order_over_the_background(self):
        base = octahedron("Sky_0", (1.0, 0.0, 0.0, 1.0))
        haze = octahedron("Sky_1", (0.0, 0.0, 1.0, 0.5))
        image = panorama(Sky((0, 255, 0), [], [base, haze]), 32, 16)
        self.assertEqual(len(image), 32 * 16 * 3)
        self.assertEqual(set(pixel(image, 32, x, y) for x in range(32) for y in range(16)), {(128, 0, 128)})
        self.assertEqual(pixel(panorama(Sky((0, 255, 0), [], []), 4, 2), 4, 0, 0), (0, 255, 0))

    def test_directions_match_godot_panorama(self):
        # Colour each octant by its direction: +Y at the left edge, +X a quarter across, +Z at the top.
        mesh = octahedron("Sky_0", (0.0, 0.0, 0.0, 1.0))
        mesh.colours = [(1, 0, 0, 1), (0, 1, 0, 1), (0, 0, 0, 1), (0, 0, 0, 1), (0, 0, 1, 1), (0, 0, 0, 1)]
        image = panorama(Sky((0, 0, 0), [], [mesh]), 64, 32)
        self.assertGreater(pixel(image, 64, 0, 16)[1], 200)   # Forward, game +Y.
        self.assertGreater(pixel(image, 64, 16, 16)[0], 200)  # Game +X.
        self.assertGreater(pixel(image, 64, 32, 0)[2], 200)   # Straight up.

    def test_textures_wrap_across_and_modulate_alpha(self):
        palette = bytearray(1024)
        palette[0:4] = bytes((255, 255, 255, 0x80))
        texture = Texture(1, 1, b"\0", bytes(palette))
        mesh = octahedron("Sky_0", (1.0, 1.0, 1.0, 0.5), ("sky", 0))
        image = panorama(Sky((0, 0, 0), [texture], [mesh]), 16, 8)
        self.assertEqual(set(pixel(image, 16, x, y) for x in range(16) for y in range(8)), {(128, 128, 128)})

    def test_footprint_covers_small_triangles_exactly(self):
        # A thin shell band: only rows near the horizon are touched.
        mesh = Mesh("Sky_0", colours=[])
        for k in range(16):
            for z in (-0.1, 0.1):
                a = math.tau * k / 16
                mesh.positions.append((math.sin(a), math.cos(a), z))
                mesh.uvs.append((0, 0))
                mesh.colours.append((1, 1, 1, 1))
        for k in range(16):
            a, b, c, d = 2 * k, 2 * k + 1, 2 * ((k + 1) % 16), 2 * ((k + 1) % 16) + 1
            mesh.add_face(None, (a, c, b))
            mesh.add_face(None, (b, c, d))
        image = panorama(Sky((0, 0, 0), [], [mesh]), 64, 32)
        self.assertEqual(pixel(image, 64, 5, 16), (255, 255, 255))
        self.assertEqual(pixel(image, 64, 5, 2), (0, 0, 0))


if __name__ == "__main__":
    unittest.main()
