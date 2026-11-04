import re
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tidy"))
import level_ids as li  # noqa: E402


class LevelIdsTest(unittest.TestCase):
    def test_header_matches_the_level_list(self):
        header = (li.ROOT / "include/enums/level.h").read_text()
        values = {name: int(value, 16) for name, value in re.findall(r"(LEVEL_\w+) = (0x[0-9A-F]+)", header)}
        for number, name in li.names().items():
            self.assertEqual(values[name], number)
        self.assertEqual(values["LEVEL_COUNT"], len(li.names()))

    def test_compare(self):
        self.assertEqual(li.rewrite("if (g_current_level_index == 5) x();"),
                         ("if (g_current_level_index == LEVEL_RILGAR) x();", 1))
        self.assertEqual(li.rewrite("if (g_current_level_index_r != 0x10) x();"),
                         ("if (g_current_level_index_r != LEVEL_KALEBO3) x();", 1))

    def test_count(self):
        self.assertEqual(li.rewrite("if (g_current_level_index < 0x13) x();"),
                         ("if (g_current_level_index < LEVEL_COUNT) x();", 1))

    def test_a_number_out_of_range_stays(self):
        text = "if (g_current_level_index == 99) x();"
        self.assertEqual(li.rewrite(text), (text, 0))

    def test_other_variables_stay(self):
        for text in ("if (level == 5) x();", "if (cd_debug_level == 5) x();", "/* g_current_level_index == 5 */"):
            self.assertEqual(li.rewrite(text), (text, 0))

    def test_switch(self):
        new, count = li.rewrite("switch (g_current_level_index) { case 1: break; case 0x12: break; }")
        self.assertEqual(count, 2)
        self.assertIn("case LEVEL_NOVALIS:", new)
        self.assertIn("case LEVEL_VELDIN2:", new)


if __name__ == "__main__":
    unittest.main()
