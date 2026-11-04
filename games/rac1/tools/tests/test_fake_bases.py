import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tidy"))
import fake_bases as fb  # noqa: E402

BASES = {"hero_globals_base": 0x1000, "D_00001000": 0x1000}
TARGETS = {0x1E1D: "g_hero", 0x1100: "hero_control_mode"}


def new(text):
    return fb.rewrite(text, BASES, TARGETS)


class RewriteTest(unittest.TestCase):
    def test_a_field_through_the_hero(self):
        text = "void f(void) {\n    ((struct Hero *)(hero_globals_base + 0xE1D))->x = 1;\n}\n"
        self.assertIn("g_hero.x = 1;", new(text)[0])
        self.assertIn("extern Hero g_hero;", new(text)[0])

    def test_a_hero_pointer(self):
        self.assertIn("Hero *p = &g_hero;", new("void f(void) {\n    Hero *p = (Hero *)(hero_globals_base + 0xE1D);\n}\n")[0])

    def test_an_int_view_of_a_global(self):
        out = new("void f(void) {\n    int a = *(int *)(hero_globals_base + 0x100);\n}\n")[0]
        self.assertIn("int a = *(int *)hero_control_mode;", out)
        self.assertIn("extern char hero_control_mode[];", out)

    def test_a_scaled_constant_stays(self):
        text = "void f(void) {\n    int a = hero_globals_base + 0x100 * i;\n}\n"
        self.assertEqual(new(text), (text, 0))

    def test_no_global_there_stays(self):
        text = "void f(void) {\n    int a = *(int *)(hero_globals_base + 0x104);\n}\n"
        self.assertEqual(new(text), (text, 0))

    def test_a_comment_stays(self):
        text = "/* hero_globals_base + 0xE1D */\nint x;\n"
        self.assertEqual(new(text), (text, 0))

    def test_a_kept_function_stays(self):
        text = "void f(void) {\n    int a = *(int *)(hero_globals_base + 0x100);\n}\n"
        self.assertEqual(fb.rewrite(text, BASES, TARGETS, {"f"}), (text, 0))

    def test_a_blocked_global_stays(self):
        text = "void f(void) {\n    int a = *(int *)(hero_globals_base + 0x100);\n}\n"
        self.assertEqual(fb.rewrite(text, BASES, TARGETS, (), {"D_00001100"}), (text, 0))

    def test_a_struct_keyword_is_not_a_disagreement(self):
        text = ("extern Hero g_hero;\nextern struct Hero g_hero;\n"
                "void f(void) {\n    Hero *p = (Hero *)(hero_globals_base + 0xE1D);\n}\n")
        self.assertEqual(new(text)[1], 1)

    def test_declarations_that_disagree_block_it(self):
        text = ("extern int hero_control_mode;\nextern short hero_control_mode;\n"
                "void f(void) {\n    int a = *(int *)(hero_globals_base + 0x100);\n}\n")
        self.assertEqual(new(text)[1], 0)

    def test_an_address_alias_blocks_it(self):
        text = ('extern int hero_control_mode __asm__("D_1");\n'
                "void f(void) {\n    int a = *(int *)(hero_globals_base + 0x100);\n}\n")
        self.assertEqual(new(text)[1], 0)


class BisectTest(unittest.TestCase):
    def test_it_keeps_all_but_the_bad_one(self):
        owners = {"a", "b", "c", "d", "e"}
        self.assertEqual(fb.accepted_functions(owners, lambda chosen: "c" not in chosen), {"a", "b", "d", "e"})

    def test_two_that_only_fail_together(self):
        owners = {"a", "b", "c"}
        out = fb.accepted_functions(owners, lambda chosen: not {"a", "c"} <= chosen)
        self.assertNotEqual(out & {"a", "c"}, {"a", "c"})
        self.assertIn("b", out)

    def test_nothing_passes(self):
        self.assertEqual(fb.accepted_functions({"a", "b"}, lambda chosen: False), set())


class ScalarTest(unittest.TestCase):
    def test_one_type_becomes_real(self):
        text = "extern char mode[];\nint f(void) { return *(int *)mode; }\n"
        self.assertEqual(fb.scalar_type(text, "mode"), "int")
        self.assertEqual(fb.give_type(text, "mode", "int"), "extern int mode;\nint f(void) { return mode; }\n")

    def test_a_small_type_stays_out_of_small_data(self):
        self.assertIn("extern short mode NOT_SDA;", fb.give_type("extern char mode[];\n", "mode", "short"))

    def test_mixed_uses_stay(self):
        self.assertIsNone(fb.scalar_type("extern char mode[];\nint a = *(int *)mode;\nshort b = *(short *)mode;\n", "mode"))
        self.assertIsNone(fb.scalar_type("extern char mode[];\nint a = *(int *)mode;\nchar *p = mode;\n", "mode"))

    def test_another_declaration_stays(self):
        self.assertIsNone(fb.scalar_type("extern int mode;\nint a = *(int *)mode;\n", "mode"))


class PlaceTest(unittest.TestCase):
    def test_declarations_go_after_the_top_includes(self):
        text = '#include "a.h"\n#include "b.h"\n\nint x;\n#include "late.h"\n'
        self.assertEqual(text[fb.after_first_includes(text):], '\nint x;\n#include "late.h"\n')

    def test_fragment_uses_the_shared_view(self):
        out = fb.rewrite("int f(void) { return *(int *)(hero_globals_base + 0x100); }\n", BASES, TARGETS,
                         scope="extern char hero_control_mode[];\n", declare=False)
        self.assertEqual(out[0], "int f(void) { return *(int *)hero_control_mode; }\n")

    def test_a_local_alias_of_a_base_counts(self):
        text = 'extern char base_r[] __asm__("D_0013E633");\nextern short other[] __asm__("D_0013E633");\n'
        self.assertEqual(fb.with_local_aliases(text, {"D_0013E633": 1}), {"D_0013E633": 1, "base_r": 1})

    def test_a_fragment_keeps_an_alias(self):
        text = 'extern char base_r[] __asm__("D_0013E633");\nint x;\n'
        self.assertEqual(fb.drop_unused_aliases(text, {"D_0013E633": 1}, "char *p = base_r;\n"), text)

    def test_an_unused_alias_goes_and_a_used_one_stays(self):
        text = 'extern char base_r[] __asm__("D_0013E633");\nint x;\n'
        self.assertEqual(fb.drop_unused_aliases(text, {"D_0013E633": 1}), "int x;\n")
        used = text + "char *p = base_r;\n"
        self.assertEqual(fb.drop_unused_aliases(used, {"D_0013E633": 1}), used)


if __name__ == "__main__":
    unittest.main()
