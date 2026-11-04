import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tidy"))
import prune_externs as pe  # noqa: E402


def prune(text, fragments="", aliases=None, macros=None):
    return pe.prune(text, fragments, aliases or {}, macros or set())


class NamesTest(unittest.TestCase):
    def test_declared_names(self):
        cases = {
            "extern int a;": ["a"],
            "extern int a[4];": ["a"],
            "extern void f(int x, char *y);": ["f"],
            "extern int (*fp)(int, int);": ["fp"],
            'extern int a __asm__("D_1");': ["a"],
            "extern int a MACRO_ADDR;": ["a"],
            "extern int a SDATA(D_1);": ["a"],
            "extern int a, b;": ["a", "b"],
            "extern struct Hero D_0013F450;": ["D_0013F450"],
            "extern Vec4 *p;": ["p"],
        }
        for statement, names in cases.items():
            with self.subTest(statement):
                self.assertEqual(pe.declared_names(statement), names)


class PruneTest(unittest.TestCase):
    def test_unused_goes_and_used_stays(self):
        new, removed, _ = prune("extern int a;\nextern int b;\nvoid f(void) { b = 1; }\n")
        self.assertEqual(removed, ["extern int a;"])
        self.assertIn("extern int b;", new)

    def test_two_copies_of_an_unused_name_both_go(self):
        _, removed, _ = prune("extern int a;\nextern int a;\nvoid f(void) { }\n")
        self.assertEqual(len(removed), 2)

    def test_two_copies_of_a_used_name_both_stay(self):
        _, removed, _ = prune("extern int a;\nextern int a;\nvoid f(void) { a = 1; }\n")
        self.assertEqual(removed, [])

    def test_a_comment_is_not_a_use(self):
        _, removed, _ = prune("extern int a;\n/* a is here */\nvoid f(void) { }\n")
        self.assertEqual(removed, ["extern int a;"])

    def test_a_string_is_a_use(self):
        _, removed, _ = prune('extern int D_1;\n__asm__("lw $v0, D_1");\n')
        self.assertEqual(removed, [])

    def test_a_version_fragment_is_a_use(self):
        _, removed, _ = prune("extern int a;\n", fragments="void g(void) { a = 1; }\n")
        self.assertEqual(removed, [])

    def test_a_header_macro_is_a_use(self):
        _, removed, _ = prune("extern int a;\n", macros={"a"})
        self.assertEqual(removed, [])

    def test_readable_and_address_names_are_the_same_symbol(self):
        aliases = {"foo": {"func_1"}, "func_1": {"foo"}}
        _, removed, _ = prune("extern void func_1(void);\nvoid f(void) { foo(); }\n", aliases=aliases)
        self.assertEqual(removed, [])

    def test_an_extern_inside_a_function_stays(self):
        _, removed, _ = prune("void f(void) {\n    extern int a;\n}\n")
        self.assertEqual(removed, [])

    def test_a_provenance_note_keeps_the_declaration(self):
        new, removed, kept = prune("extern int a; /* Adapted from Lombyte's project (GPL-3.0) */\n")
        self.assertEqual((removed, kept), ([], 1))
        self.assertIn("Adapted", new)

    def test_a_used_name_in_a_multi_name_declaration_keeps_it(self):
        _, removed, _ = prune("extern int a, b;\nvoid f(void) { b = 1; }\n")
        self.assertEqual(removed, [])

    def test_a_directive_is_not_parsed_as_a_declaration(self):
        new, removed, _ = prune("#define DECL extern int a;\nvoid f(void) { }\n")
        self.assertEqual(removed, [])
        self.assertIn("#define DECL", new)

    def test_trailing_comment_goes_with_the_declaration(self):
        new, removed, _ = prune("extern int a; /* old note */\nvoid f(void) { }\n")
        self.assertEqual(len(removed), 1)
        self.assertNotIn("old note", new)


if __name__ == "__main__":
    unittest.main()
