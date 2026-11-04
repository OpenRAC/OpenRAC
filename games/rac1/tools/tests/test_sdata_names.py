import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tidy"))
import sdata_names as sn  # noqa: E402

NAMES = {"jump_speed": {"D_L17_00162108"}, "two": {"D_00000001", "D_00000002"}}


class SdataNamesTest(unittest.TestCase):
    def test_the_name_replaces_its_address(self):
        text = "extern float jump_speed SDATA(D_L17_00162108);\n"
        self.assertEqual(sn.rewrite(text, NAMES), ("extern float jump_speed SDATA(jump_speed);\n", 1))

    def test_a_name_for_another_address_stays(self):
        text = "extern float jump_speed SDATA(D_L17_00000000);\n"
        self.assertEqual(sn.rewrite(text, NAMES), (text, 0))

    def test_a_private_alias_stays(self):
        text = "extern float jump_speed_r SDATA(D_L17_00162108);\n"
        self.assertEqual(sn.rewrite(text, NAMES), (text, 0))

    def test_a_name_with_two_addresses_stays(self):
        text = "extern float two SDATA(D_00000001);\n"
        self.assertEqual(sn.rewrite(text, NAMES), (text, 0))


if __name__ == "__main__":
    unittest.main()
