import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tidy"))
import drop_aliases as da  # noqa: E402

NAMES = {"D_00131418": {"level"}, "level": {"D_00131418"}}
SOURCE = 'extern int level_r[] __asm__("D_00131418");\nint f(void) { return level_r[1]; }\n'


def plan(text, fragments="", in_headers=frozenset()):
    return da.plan(text, fragments, NAMES, set(in_headers))


class DropAliasesTest(unittest.TestCase):
    def test_the_only_view_becomes_plain(self):
        edits = plan(SOURCE)
        self.assertEqual(da.apply_plan(SOURCE, edits), "extern int D_00131418[];\nint f(void) { return level[1]; }\n")

    def test_a_symbol_with_no_name_keeps_its_alias(self):
        self.assertEqual(plan('extern int a_r __asm__("D_00999999");\nint f(void) { return a_r; }\n'), [])

    def test_a_second_view_keeps_the_alias(self):
        self.assertEqual(plan(SOURCE + "extern short level;\n"), [])
        self.assertEqual(plan(SOURCE + "int g(void) { return D_00131418; }\n"), [])

    def test_a_fragment_keeps_the_alias(self):
        self.assertEqual(plan(SOURCE, fragments="int v(void) { return level_r[0]; }\n"), [])
        self.assertEqual(plan(SOURCE, fragments="int v(void) { return level; }\n"), [])

    def test_a_header_keeps_the_alias(self):
        self.assertEqual(plan(SOURCE, in_headers={"level"}), [])
        self.assertEqual(plan(SOURCE, in_headers={"D_00131418"}), [])

    def test_a_gp_label_stays(self):
        self.assertEqual(plan('extern int a_r __asm__("D_00131418__gp");\nint x;\n'), [])

    def test_a_qualifier_stays_on_the_declaration(self):
        text = 'extern volatile int level_v __asm__("D_00131418");\nint f(void) { return level_v; }\n'
        self.assertIn("extern volatile int D_00131418;", da.apply_plan(text, plan(text)))


if __name__ == "__main__":
    unittest.main()
