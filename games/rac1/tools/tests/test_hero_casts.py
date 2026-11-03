import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tidy"))
import hero_casts as hc  # noqa: E402

SOURCE = ("extern Hero g_hero;\nint f(void) {\n    return ((Hero *)&g_hero)->a;\n}\n"
          "int g(void) {\n    return ((struct Hero *)&g_hero)->b.c + 1;\n}\n")


class HeroCastTest(unittest.TestCase):
    def test_sites_by_function(self):
        self.assertEqual(hc.owners(SOURCE), {"f": 1, "g": 1})

    def test_only_chosen_functions_change(self):
        new = hc.apply(SOURCE, {"g"})
        self.assertIn("return ((Hero *)&g_hero)->a;", new)
        self.assertIn("return g_hero.b.c + 1;", new)

    def test_a_byte_array_declaration_stays(self):
        self.assertEqual(hc.owners("extern char g_hero[];\nint f(void) {\n    return ((Hero *)&g_hero)->a;\n}\n"), {})

    def test_a_declaration_in_a_comment_does_not_count(self):
        self.assertEqual(hc.owners("/* extern Hero g_hero; */\nint f(void) {\n    return ((Hero *)&g_hero)->a;\n}\n"), {})


if __name__ == "__main__":
    unittest.main()
