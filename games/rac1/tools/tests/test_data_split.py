import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tidy"))
import data_split as ds  # noqa: E402


class UsedAddressesTest(unittest.TestCase):
    NAMES = {"base": 0x1000, "speed": 0x2000}

    def test_each_form(self):
        used = ds.used_addresses(["int a = D_00003000; int b = speed; int c = base + 0x10;"], self.NAMES)
        self.assertEqual(used, {0x3000, 0x2000, 0x1000, 0x1010})

    def test_a_comment_does_not_count(self):
        self.assertEqual(ds.used_addresses(["/* D_00003000 speed */"], self.NAMES), set())

    def test_a_scaled_constant_does_not_count(self):
        self.assertNotIn(0x1010, ds.used_addresses(["int c = base + 0x10 * i;"], self.NAMES))


class SplitPointsTest(unittest.TestCase):
    def test_on_8_starts_a_block(self):
        self.assertEqual(ds.split_points(0x100, 0x40, {0x110}), [0x110])

    def test_one_word_on_4_starts_a_block(self):
        self.assertEqual(ds.split_points(0x100, 0x40, {0x114, 0x118}), [0x114, 0x118])

    def test_more_than_a_word_on_4_stays_inside(self):
        self.assertEqual(ds.split_points(0x100, 0x40, {0x114, 0x120}), [0x120])

    def test_the_edges_and_odd_addresses_stay_out(self):
        self.assertEqual(ds.split_points(0x100, 0x40, {0x100, 0x140, 0x111}), [])


if __name__ == "__main__":
    unittest.main()
