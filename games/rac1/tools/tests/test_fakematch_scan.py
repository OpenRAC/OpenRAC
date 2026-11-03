import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import fakematch_scan as fs  # noqa: E402


class HitsTest(unittest.TestCase):
    def test_volatile_register_and_if_zero_in_a_function(self):
        text = "void f(void) {\n    volatile int a;\n    register int b;\n    if (0) {\n    }\n}\n"
        self.assertEqual(fs.line_hits(text), [(2, "volatile"), (3, "register"), (4, "if_zero")])

    def test_hardware_addresses_stay_out(self):
        text = "void f(void) {\n    *(volatile u32 *)0x10003000 = 1;\n}\n"
        self.assertEqual(fs.line_hits(text), [])

    def test_a_struct_field_outside_a_function_stays_out(self):
        self.assertEqual(fs.line_hits("struct S {\n    volatile int wr;\n};\n"), [])

    def test_inline_asm_stays_out(self):
        text = 'void f(void) {\n    __asm__ volatile("nop");\n    register float k __asm__("$f4") = 1.0f;\n}\n'
        self.assertEqual(fs.line_hits(text), [])

    def test_a_kept_comment_settles_the_site(self):
        text = "void f(void) {\n    /* KEPT: the store stays out of the delay slot. */\n    volatile int a;\n}\n"
        self.assertEqual(fs.line_hits(text), [])
        self.assertEqual(fs.table_rows("src/a.c", text), [("src/a.c", "f", "kept", "the store stays out of the delay slot")])

    def test_comments_stay_out(self):
        self.assertEqual(fs.line_hits("void f(void) {\n    /* volatile if (0) register */\n}\n"), [])


class TagsTest(unittest.TestCase):
    def test_a_tag_goes_above_at_the_same_indent(self):
        new, added = fs.add_tags("void f(void) {\n    volatile int a;\n}\n", [(2, "volatile")])
        self.assertEqual(added, 1)
        self.assertEqual(new.split("\n")[1], f"    /* {fs.TAG} {fs.REASONS['volatile']}. */")
        self.assertEqual(new.split("\n")[2], "    volatile int a;")

    def test_tagging_twice_changes_nothing(self):
        once, _ = fs.add_tags("void f(void) {\n    volatile int a;\n}\n", [(2, "volatile")])
        twice, added = fs.add_tags(once, fs.line_hits(once))
        self.assertEqual((twice, added), (once, 0))

    def test_old_comments_get_the_one_spelling(self):
        self.assertEqual(fs.mark_existing("/* Fakematch: the goto pair. */"), f"/* {fs.TAG} the goto pair. */")


class RowsTest(unittest.TestCase):
    def test_rows(self):
        text = f"void f(void) {{\n    /* {fs.TAG} {fs.REASONS['volatile']}. */\n    volatile int a;\n}}\n"
        self.assertEqual(fs.table_rows("src/a.c", text), [("src/a.c", "f", "volatile", fs.REASONS["volatile"])])

    def test_a_marked_comment_keeps_its_reason(self):
        text = f"void f(void) {{\n    /* {fs.TAG} the xor test. */\n}}\n"
        self.assertEqual(fs.table_rows("src/a.c", text), [("src/a.c", "f", "marked", "the xor test")])

    def test_padding_is_listed_but_not_tagged(self):
        text = '__asm__(".section .text\\n"\n        "\\tnop\\n");\n'
        self.assertEqual(fs.table_rows("src/a.c", text), [("src/a.c", "-", "padding", fs.REASONS["padding"])])


if __name__ == "__main__":
    unittest.main()
