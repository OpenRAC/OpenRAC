import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tidy"))
import comment_names as cn  # noqa: E402

NAMES = {"func_001F4E08": "fade_to_black", "D_0013F450": "g_hero"}


def rewrite(text):
    return cn.rewrite(text, NAMES)


class CommentNamesTest(unittest.TestCase):
    def test_a_name_replaces_the_address(self):
        self.assertEqual(rewrite("/* Calls func_001F4E08 with D_0013F450. */"), ("/* Calls fade_to_black with g_hero. */", 1))

    def test_a_symbol_with_no_name_stays(self):
        text = "/* Calls func_00112380. */"
        self.assertEqual(rewrite(text), (text, 0))

    def test_code_and_strings_stay(self):
        text = 'func_001F4E08(); char *s = "func_001F4E08";'
        self.assertEqual(rewrite(text), (text, 0))

    def test_provenance_stays(self):
        text = "/* Calls func_001F4E08. Adapted from Lombyte's project (GPL-3.0): func_001F4E08. */"
        self.assertEqual(rewrite(text)[0], "/* Calls fade_to_black. Adapted from Lombyte's project (GPL-3.0): func_001F4E08. */")

    def test_a_repeated_name_goes(self):
        self.assertEqual(rewrite("/* func_001F4E08 (fade_to_black) runs. */")[0], "/* fade_to_black runs. */")

    def test_the_status_tag_goes_and_the_description_stays(self):
        self.assertEqual(rewrite("/* functional: medium. Draw callback. */")[0], "/* Draw callback. */")

    def test_spelling(self):
        self.assertEqual(rewrite("/* Update of Skid Mcmarx. */")[0], "/* Update of Skid McMarx. */")

    def test_line_comments_too(self):
        self.assertEqual(rewrite("// Calls func_001F4E08\n")[0], "// Calls fade_to_black\n")

    def test_a_word_in_a_longer_name_stays(self):
        text = "/* my_func_001F4E08x */"
        self.assertEqual(rewrite(text), (text, 0))


class WrapTest(unittest.TestCase):
    LONG = "word " * 40

    def test_a_block_comment_wraps_under_its_text(self):
        lines = cn.wrap_line("    /* " + self.LONG.strip() + " */")
        self.assertGreater(len(lines), 1)
        self.assertTrue(all(len(line) <= 120 for line in lines))
        self.assertTrue(lines[1].startswith("       word"))

    def test_a_star_line_keeps_its_star(self):
        lines = cn.wrap_line(" * " + self.LONG.strip())
        self.assertTrue(lines[1].startswith(" * word"))

    def test_words_are_all_kept(self):
        lines = cn.wrap_line("/* " + self.LONG.strip() + " */")
        self.assertEqual(" ".join(lines).replace("   ", " ").split(), ("/* " + self.LONG + "*/").split())

    def test_provenance_and_markers_stay(self):
        for line in ("/* Adapted from Lombyte's project " + self.LONG + "*/", "/* KEPT: " + self.LONG + "*/"):
            self.assertEqual(cn.wrap_line(line), [line])

    def test_a_trailing_comment_stays(self):
        text = "int a; /* " + self.LONG + "*/"
        self.assertEqual(cn.wrap_comments(text), text)

    def test_a_short_line_stays(self):
        self.assertEqual(cn.wrap_line("/* short */"), ["/* short */"])


if __name__ == "__main__":
    unittest.main()
