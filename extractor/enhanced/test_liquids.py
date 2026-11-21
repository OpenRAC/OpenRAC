import hashlib
import json
import struct
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

from enhanced import liquids


def gameplay_with_pvars(blocks: list[bytes]) -> bytes:
    """A 0xA0-byte Going Commando header with only the pvar table (0x5C) and data (0x60) placed."""
    header = bytearray(0xA0)
    table = len(header)
    data = table + 8 * len(blocks)
    struct.pack_into("<2i", header, liquids.PVAR_TABLE, table, data)
    entries, body = b"", b""
    for block in blocks:
        entries += struct.pack("<2i", len(body), len(block))
        body += block
    return bytes(header) + entries + body


def surface_pvar(texture: int, flags: int = 3, repeat: float = 0.6) -> bytes:
    state = bytearray(0x80)
    struct.pack_into("<f", state, 0x28, repeat)
    struct.pack_into("<2I", state, 0x48, texture, flags)
    return bytes(state)


def marker_pvar(selector: int) -> bytes:
    settings = bytearray(0x20)
    struct.pack_into("<I", settings, 0x14, selector)
    return bytes(settings)


def moby(index, class_id, group, pvar, x, y, z):
    matrix = [1.0, 0, 0, 0, 0, 1.0, 0, 0, 0, 0, 1.0, 0, x, y, z, 1.0]
    return {"index": index, "class_id": class_id, "matrix": matrix,
            "fields": {"group": group, "pvar_index": pvar}}


def effect_core(number: int, colour: tuple[int, int, int]) -> tuple[bytes, bytes]:
    """A core index whose effect texture NUMBER is 2 x 2 pixels of one palette colour."""
    index = bytearray(0x100)
    table = 0xA0
    struct.pack_into("<2I", index, liquids.EFFECT_TABLE, number + 1, table)
    struct.pack_into("<I", index, liquids.EFFECT_BANK, 16)
    index += bytes(16 * (number + 1))
    struct.pack_into("<4i", index, table + 16 * number, 0, 1024, 2, 2)
    palette = bytearray(1024)
    palette[4:8] = bytes([*colour, 0x80])
    core = bytes(16) + bytes(palette) + bytes([1, 1, 1, 1])
    return bytes(index), core


class Liquids(unittest.TestCase):
    def level(self, mobys, blocks, colour=(255, 128, 0)):
        index, core = effect_core(2 + liquids.EFFECT_BASE, colour)
        level = SimpleNamespace(gameplay=gameplay_with_pvars(blocks), index=index, core=core)
        return level, patch.object(liquids, "moby_instances", return_value=mobys)

    def surface_level(self, colour=(255, 128, 0)):
        corners = [(362, 224), (363, 188), (347, 190), (347, 223)]
        # Markers listed out of selector order: the selector, not the order, names the corner.
        order = [2, 0, 3, 1]
        mobys = [moby(0, liquids.SURFACE_CLASS, 33, 0, 353, 196, 133)]
        blocks = [surface_pvar(2)]
        for n, selector in enumerate(order):
            x, y = corners[selector]
            mobys.append(moby(n + 1, liquids.CONTROL_CLASS, 33, n + 1, x, y, 130.0 + (selector != 0)))
            blocks.append(marker_pvar(selector))
        mobys.append(moby(9, liquids.CONTROL_CLASS, 34, 5, 0, 0, 0))  # another group's
        blocks.append(marker_pvar(0))
        return self.level(mobys, blocks, colour), corners

    def test_corners_by_selector_and_height_from_the_first(self):
        (level, patched), corners = self.surface_level()
        with patched:
            found = liquids.surfaces(level, None)
        self.assertEqual(len(found), 1)
        self.assertEqual(found[0]["corners"], [list(c) for c in corners])
        self.assertEqual(found[0]["height"], 130.0)
        self.assertEqual(found[0]["texture"], 2 + liquids.EFFECT_BASE)
        self.assertAlmostEqual(found[0]["repeat"], 0.6, places=6)

    def test_a_surface_without_four_markers_has_no_grid(self):
        level, patched = self.level([moby(0, liquids.SURFACE_CLASS, 7, 0, 0, 0, 0)], [surface_pvar(2)])
        with patched:
            self.assertEqual(liquids.surfaces(level, None), [])

    def test_only_reviewed_textures_emit(self):
        (level, patched), _ = self.surface_level()
        with patched, tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.assertEqual(liquids.write_liquids(level, None, root, set()), {"liquids": "rac2-surfaces-v1"})
            written = json.loads((root / "liquids.json").read_text())["surfaces"][0]
            self.assertFalse(written["emissive"])
            self.assertNotIn("emission", written)
            image = (root / written["image"]).read_bytes()
            reviewed = {hashlib.sha256(image).hexdigest()}
            liquids.write_liquids(level, None, root, reviewed)
            written = json.loads((root / "liquids.json").read_text())["surfaces"][0]
            self.assertTrue(written["emissive"])
            # 255, 128, 0 at the rest tint 80/128, averaged in linear light, back in display units.
            expected = [liquids.display(liquids.linear(c / 255 * 80 / 128)) for c in (255, 128, 0)]
            for got, want in zip(written["emission"], expected):
                self.assertAlmostEqual(got, want, places=5)

    def test_no_surfaces_writes_nothing(self):
        level, patched = self.level([], [])
        with patched, tempfile.TemporaryDirectory() as temporary:
            self.assertIsNone(liquids.write_liquids(level, None, Path(temporary), set()))
            self.assertEqual(list(Path(temporary).iterdir()), [])


if __name__ == "__main__":
    unittest.main()
