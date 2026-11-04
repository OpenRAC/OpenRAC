import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tidy"))
import hero_pointers as hp  # noqa: E402

SOURCE = "void f(void) {\n    char *p = g_hero;\n    ((struct Hero *)p)->a = 1;\n    use((struct Hero *)p);\n}\n"


class HeroPointerTest(unittest.TestCase):
    def test_every_cast_goes(self):
        self.assertEqual(hp.apply(SOURCE, hp.sites(SOURCE)),
                         "void f(void) {\n    struct Hero *p = (struct Hero *)g_hero;\n    p->a = 1;\n    use(p);\n}\n")

    def test_the_address_form_keeps_its_address(self):
        text = "void f(void) {\n    char *p = (char *)&g_hero;\n    ((struct Hero *)p)->a = 1;\n}\n"
        self.assertIn("struct Hero *p = (struct Hero *)&g_hero;", hp.apply(text, hp.sites(text)))

    def test_other_uses_stay(self):
        for use in ("x = *(int *)(p + 8);", "q = p;"):
            text = f"void f(void) {{\n    char *p = g_hero;\n    ((struct Hero *)p)->a = 1;\n    {use}\n}}\n"
            self.assertEqual(hp.sites(text), [])

    def test_a_name_in_a_comment_does_not_count(self):
        text = "void f(void) {\n    char *p = g_hero;\n    ((struct Hero *)p)->a = 1; /* p + 8 */\n}\n"
        self.assertEqual(len(hp.sites(text)), 1)

    def test_blocks_are_separate(self):
        text = ("void f(void) {\n    {\n        char *p = g_hero;\n        ((struct Hero *)p)->a = 1;\n    }\n"
                "    {\n        char *p = g_hero;\n        x = p + 8;\n    }\n}\n")
        self.assertEqual(len(hp.sites(text)), 1)


if __name__ == "__main__":
    unittest.main()
