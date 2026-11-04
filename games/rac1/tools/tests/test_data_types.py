import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import data_types as dt  # noqa: E402

DUMP = """
*** Dumping AST Record Layout
         0 | struct P
         0 |   int a
         4 |   float b
         8 |   char *c
        12 |   u8[4] d
        16 |   struct Q q
        16 |     short x
        18 |     short y
           | [sizeof=20, align=4]
"""


class DataTypesTest(unittest.TestCase):
    def test_records_and_leaves(self):
        records = dt.parse_records(DUMP)
        self.assertEqual(records["struct P"][0], 20)
        out = []
        dt.flatten(records["struct P"][1], 0, 1, 0, records, out)
        self.assertEqual(out, [(0, "i", 4), (4, "f", 4), (8, "p", 4), (12, "i", 1), (13, "i", 1), (14, "i", 1), (15, "i", 1),
                               (16, "i", 2), (18, "i", 2)])

    def test_a_union_is_refused(self):
        records = dt.parse_records("*** Dumping AST Record Layout\n 0 | struct U\n 0 |   union V v\n 0 |     int a\n | [sizeof=4, align=4]\n")
        with self.assertRaises(ValueError):
            dt.flatten(records["struct U"][1], 0, 1, 0, records, [])

    def test_an_unknown_type_is_refused(self):
        with self.assertRaises(ValueError):
            dt.leaf("Mystery", 0, 0, {}, [])


if __name__ == "__main__":
    unittest.main()
