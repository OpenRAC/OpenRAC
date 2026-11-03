import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tidy"))
import control_modes as cm  # noqa: E402


class ControlModeTest(unittest.TestCase):
    def test_compare_and_store_get_names(self):
        self.assertEqual(cm.rewrite("if (g->state.control_mode == 0x14) x.state.control_mode = 4;")[0],
                         "if (g->state.control_mode == HERO_MODE_DYING) x.state.control_mode = HERO_MODE_JUMP;")

    def test_decimal_and_prev_fields(self):
        self.assertEqual(cm.rewrite("a.state.prev_control_mode != 15;")[0],
                         "a.state.prev_control_mode != HERO_MODE_GRIND;")

    def test_the_global_counts(self):
        self.assertEqual(cm.rewrite("hero_control_mode == 0xF;")[0], "hero_control_mode == HERO_MODE_GRIND;")
        self.assertEqual(cm.rewrite("hero_control_mode[0] == 0x1A)")[0], "hero_control_mode[0] == HERO_MODE_FOLLOW_PATH)")

    def test_a_value_with_no_name_stays(self):
        self.assertEqual(cm.rewrite("a.state.control_mode == 9;"), ("a.state.control_mode == 9;", 0))

    def test_arithmetic_stays(self):
        for text in ("(a.state.control_mode - 0x15) < 2;", "a.state.control_mode == 4 + b;"):
            self.assertEqual(cm.rewrite(text), (text, 0))

    def test_comments_and_strings_stay(self):
        text = '/* a.control_mode == 4; */ char *s = "a.control_mode = 4;";'
        self.assertEqual(cm.rewrite(text), (text, 0))

    def test_a_line_end_counts(self):
        self.assertEqual(cm.rewrite("x = a.control_mode == 2\n  || y;")[1], 1)

    def test_the_include_goes_after_the_last_one(self):
        self.assertEqual(cm.add_include('#include "a.h"\n#include "b.h"\nint x;\n'),
                         '#include "a.h"\n#include "b.h"\n#include "enums/hero_state.h"\nint x;\n')
        self.assertEqual(cm.add_include("int x;\n"), '#include "enums/hero_state.h"\nint x;\n')


if __name__ == "__main__":
    unittest.main()
