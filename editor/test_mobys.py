"""Synthetic moby instance records; no disc data."""

import math
import struct
import unittest

from formats import FormatError
from godot import write_marker
from mobys import POINTER, RECORD, SIZE, USUAL, matrix, moby_class_names, moby_instances


def record(**changes):
    """A 0x78-byte instance with the usual values, as most on the disc are."""
    values = {"size": SIZE, "unknown_4": -1, "spawn_flags": 0, "spawn_id": 7, "unknown_10": 0, "unknown_14": 0,
              "class_id": 11, "scale": 1.0, "draw_distance": 64, "update_distance": 64, "unused_28": 32,
              "unused_2c": 64, "x": 1.0, "y": 2.0, "z": 3.0, "rx": 0.0, "ry": 0.0, "rz": 0.0, "group": -1,
              "is_rooted": 0, "rooted_distance": 0.0, "unknown_54": 1, "pvar_index": 4, "occlusion": 1,
              "mode_bits": 32, "r": 31, "g": 26, "b": 21, "light": 0, "unknown_74": -1}
    values.update(changes)
    return RECORD.pack(*values.values())


def gameplay(*records, at=0x100):
    data = bytearray(at) + struct.pack("<i12x", len(records)) + b"".join(records)
    struct.pack_into("<I", data, POINTER, at)
    return bytes(data)


class MobyTests(unittest.TestCase):
    def test_usual_values_stay_out_of_the_metadata(self):
        moby, = moby_instances(gameplay(record()))
        self.assertEqual(moby["class_id"], 11)
        self.assertEqual(moby["fields"], {"spawn_id": 7, "group": -1, "pvar_index": 4, "colour": [31, 26, 21]})
        self.assertEqual(moby["matrix"][12:15], [1.0, 2.0, 3.0])

    def test_unusual_values_are_kept_for_a_packer(self):
        moby, = moby_instances(gameplay(record(spawn_flags=3, light=0x0201, unknown_74=5, rooted_distance=2.5)))
        for key, value in {"spawn_flags": 3, "light": 0x0201, "unknown_74": 5, "rooted_distance": 2.5}.items():
            self.assertEqual(moby["fields"][key], value)
        self.assertTrue(set(moby["fields"]) - {"spawn_id", "group", "pvar_index", "colour"} <= set(USUAL))

    def test_rotation_is_rz_ry_rx_then_scale(self):
        m = matrix((0, 0, 0), (0.0, 0.0, math.pi / 2), 2.0)
        # Rz(90 degrees) takes +x to +y; the columns are scaled by 2.
        self.assertAlmostEqual(m[1], 2.0)
        self.assertAlmostEqual(m[0], 0.0)
        m = matrix((0, 0, 0), (math.pi / 2, 0.0, math.pi / 2), 1.0)
        # x first: Rx(90) takes +y to +z, then Rz leaves +z alone.
        self.assertAlmostEqual(m[4 + 2], 1.0)

    def test_empty_and_bad_tables(self):
        self.assertEqual(moby_instances(bytes(0x100)), [])
        with self.assertRaises(FormatError):
            moby_instances(gameplay(record(size=0x70)))
        with self.assertRaises(FormatError):
            moby_instances(gameplay(record())[:-8])

    def test_class_names_and_marker(self):
        self.assertEqual(moby_class_names()[11], "vendor")
        import tempfile
        from pathlib import Path
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "mobys" / "moby_11.tscn"
            write_marker(path, 11, "11 vendor")
            text = path.read_text()
        self.assertIn('text = "11 vendor"', text)
        self.assertIn("metadata/rc1_class = 11", text)


if __name__ == "__main__":
    unittest.main()
