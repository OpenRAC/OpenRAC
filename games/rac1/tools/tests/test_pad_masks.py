import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tidy"))
import pad_masks as pm  # noqa: E402


class SpelledTest(unittest.TestCase):
    def test_one_button(self):
        self.assertEqual(pm.spelled(0x40), "PAD_CROSS")
        self.assertEqual(pm.spelled(0x8000), "PAD_LEFT")

    def test_several_buttons(self):
        self.assertEqual(pm.spelled(0xC0), "(PAD_CROSS | PAD_SQUARE)")

    def test_not_buttons(self):
        self.assertIsNone(pm.spelled(0))
        self.assertIsNone(pm.spelled(0x10000))
        self.assertIsNone(pm.spelled(0xFFFF))

    def test_every_name_matches_the_header(self):
        header = (pm.ROOT / "include/structs/pad.h").read_text()
        for bit, name in pm.BITS.items():
            self.assertIn(f"#define {name} 0x{bit:04X}", header)


class RewriteTest(unittest.TestCase):
    def test_the_pad_is_rewritten(self):
        self.assertEqual(pm.rewrite("if (g_controller_state.pressed & 0x40) x();"),
                         ("if (g_controller_state.pressed & PAD_CROSS) x();", 1))
        self.assertEqual(pm.rewrite("if (pad0_pressed & 0xC0) x();"), ("if (pad0_pressed & (PAD_CROSS | PAD_SQUARE)) x();", 1))

    def test_other_variables_stay(self):
        for text in ("if (held & 0x40) x();", "if (m->pressed & 0x40) x();", "if (key_pressed & 0x40) x();"):
            self.assertEqual(pm.rewrite(text), (text, 0))

    def test_comments_stay(self):
        text = "/* g_controller_state.pressed & 0x40 */"
        self.assertEqual(pm.rewrite(text), (text, 0))


if __name__ == "__main__":
    unittest.main()
