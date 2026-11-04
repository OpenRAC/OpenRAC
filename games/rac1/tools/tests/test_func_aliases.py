import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tidy"))
import func_aliases as fa  # noqa: E402

TEXT = 'extern void vsub_x(float *, void *) __asm__("func_001F9BF0");\nvoid f(void) { vsub_x(0, 0); } /* vsub_x */\n'


class Fake:
    aliases = {"func_001F9BF0": {"FastVecSub"}, "FastVecSub": {"func_001F9BF0"}}
    defined = {}
    blocked = {}

    def symbol(self, name):
        return min({name} | self.aliases.get(name, set()))


class AliasTest(unittest.TestCase):
    def test_lines_are_found(self):
        rows = fa.alias_lines(TEXT)
        self.assertEqual([(r[2], r[3]) for r in rows], [("vsub_x", "func_001F9BF0")])

    def test_a_data_alias_is_not_a_function(self):
        self.assertEqual(fa.alias_lines('extern char a[] __asm__("D_00100000");\n'), [])

    def test_a_matching_alias_takes_the_real_name(self):
        wanted = {Fake().symbol("func_001F9BF0"): ("void *", "void *")}
        found = fa.matching(TEXT, Fake(), wanted, "a.c")
        self.assertEqual(found, {"vsub_x": ("func_001F9BF0", "FastVecSub")})
        self.assertEqual(fa.apply(TEXT, found),
                         "extern void FastVecSub(float *, void *);\nvoid f(void) { FastVecSub(0, 0); } /* vsub_x */\n")

    def test_another_order_stays(self):
        wanted = {Fake().symbol("func_001F9BF0"): ("float", "void *")}
        self.assertEqual(fa.matching(TEXT, Fake(), wanted, "a.c"), {})

    def test_the_defining_file_stays(self):
        fake = Fake()
        fake.defined = {fake.symbol("func_001F9BF0"): {"a.c"}}
        self.assertEqual(fa.matching(TEXT, fake, {fake.symbol("func_001F9BF0"): ("void *", "void *")}, "a.c"), {})


if __name__ == "__main__":
    unittest.main()
