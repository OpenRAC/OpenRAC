import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tidy"))
import raw_views as rv  # noqa: E402


def plan(body):
    return rv.plan("void f(void) {\n" + body + "\n}\n")


class PlanTest(unittest.TestCase):
    def test_each_source(self):
        self.assertEqual(plan("char *m = CreateMoby(1);\n*(int *)(m + 4) = 1;"), [("create_moby", "f", "m")])
        self.assertEqual(plan("char *h = (char *)&g_hero;\n*(int *)(h + 4) = 1;"), [("hero", "f", "h")])
        self.assertEqual(plan("char *r = MobyGetHitMessage(a, 1, 0);\n*(int *)(r + 4) = 1;"), [("hit", "f", "r")])
        self.assertEqual(plan("char *d = (char *)((Moby *)a)->pvars;\n*(int *)(d + 4) = 1;"), [("pvars", "f", "d")])

    def test_every_spelling_of_pvars(self):
        for value in ("((char *)((Moby *)a)->pvars)", "a->pvars", "*(char **)(a + 0x78)", "g_hero.target->pvars"):
            self.assertEqual(plan(f"char *d = {value};\n*(int *)(d + 4) = 1;"), [("pvars", "f", "d")], value)

    def test_a_field_of_pvars_is_not_pvars(self):
        self.assertEqual(plan("char *d = *(char **)(a->pvars + 4);\n*(int *)(d + 4) = 1;"), [])

    def test_a_second_value_rules_it_out(self):
        self.assertEqual(plan("char *m = CreateMoby(1);\nm = other;\n*(int *)(m + 4) = 1;"), [])

    def test_a_typed_pointer_is_left(self):
        self.assertEqual(plan("Other *m = CreateMoby(1);\n*(int *)(m + 4) = 1;"), [])

    def test_only_picks_one_rule(self):
        text = "void f(void) {\nchar *m = CreateMoby(1);\n*(int *)(m + 4) = 1;\n}\n"
        self.assertEqual(rv.plan(text, "hero"), [])


class NameTest(unittest.TestCase):
    def test_struct_name(self):
        self.assertEqual(rv.struct_name("update_falling_splash"), "UpdateFallingSplashVars")
        self.assertEqual(rv.struct_name("func_L00_0025B478"), "L000025B478Vars".replace("L000025", "L00" + "0025"))

    def test_a_struct_goes_before_the_comment(self):
        text = "int x;\n\n/* Does it. */\nvoid f(char *m) {\n}\n"
        self.assertEqual(text[rv.definition_start(text, "f"):].split("\n")[0], "/* Does it. */")


if __name__ == "__main__":
    unittest.main()
