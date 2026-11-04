import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tidy"))
import move_externs as me  # noqa: E402


def proto(name, statement):
    return me.split_prototype(name, statement)


class ShapeTest(unittest.TestCase):
    def test_pointers_are_one_type(self):
        self.assertEqual(me.shape(proto("g", "extern void g(float *, Moby *a);")), ("void *", "void *"))

    def test_spellings_of_a_scalar_match(self):
        self.assertEqual(me.shape(proto("g", "extern s32 g(s32 x, f32);")), ("int", "float"))
        self.assertEqual(me.shape(proto("g", "extern int g(int, float y);")), ("int", "float"))

    def test_width_is_kept(self):
        self.assertNotEqual(me.shape(proto("g", "extern void g(short);")), me.shape(proto("g", "extern void g(int);")))

    def test_no_parameters_is_not_void(self):
        self.assertIsNone(me.shape(proto("g", "extern void g();")))
        self.assertEqual(me.shape(proto("g", "extern void g(void);")), ("void",))


class CanonicalTest(unittest.TestCase):
    def test_the_majority_wins_and_a_value_return_beats_void(self):
        rows = [proto("g", "extern void g(int);"), proto("g", "extern void g(int);"), proto("g", "extern int g(s32);"),
                proto("g", "extern void g(float);")]
        self.assertEqual(me.canonical("g", rows), "extern int g(int);")

    def test_a_pointer_return(self):
        self.assertEqual(me.canonical("g", [proto("g", "extern Moby *g(int);")]), "extern void *g(int);")

    def test_only_empty_parentheses_give_nothing(self):
        self.assertIsNone(me.canonical("g", [proto("g", "extern void g();")]))


class DefinitionTest(unittest.TestCase):
    def test_types_stay_and_names_go(self):
        self.assertEqual(me.definition_prototype("g", "Moby *g(Moby *m, int n, float v[]) {\n}\n"),
                         ("Moby *", ["Moby *", "int", "float *"]))

    def test_a_static_or_a_declaration_is_not_a_definition(self):
        self.assertIsNone(me.definition_prototype("g", "static int g(int a) {\n}\n"))
        self.assertIsNone(me.definition_prototype("g", "extern int g(int a);\n"))

    def test_void_stays(self):
        self.assertEqual(me.definition_prototype("g", "void g(void) {\n}\n"), ("void", ["void"]))


class VariantTest(unittest.TestCase):
    class Fake:
        aliases = {"Approach": {"func_00214D28"}, "func_00214D28": {"Approach"}}

    def test_a_file_keeps_its_own_order_under_another_name(self):
        text = "extern float Approach(float, float *);\nfloat f(float *p) { return Approach(1.0f, p); } /* Approach */\n"
        self.assertEqual(me.use_variant(text, self.Fake, "Approach"),
                         'extern float Approach_v(float, float *) __asm__("func_00214D28");\n'
                         "float f(float *p) { return Approach_v(1.0f, p); } /* Approach */\n")

    def test_no_address_means_no_alias(self):
        self.assertIsNone(me.use_variant("extern int g(int);\n", type("F", (), {"aliases": {}}), "g"))


class DeclarationsTest(unittest.TestCase):
    def names(self, text):
        return [statement for _, _, statement in me.declarations(text)]

    def test_a_prototype_without_extern_counts(self):
        self.assertEqual(self.names("void g(float, void *);\nextern int h(void);\n"),
                         ["void g(float, void *);", "extern int h(void);"])
        self.assertEqual(me.split_prototype("g", "void g(float, void *);"), ("void", ["float", "void *"]))

    def test_a_macro_call_or_a_definition_does_not(self):
        self.assertEqual(self.names('INCLUDE_ASM("asm/overlays", func_00100000);\nint g(void) {\n    return f(1);\n}\n'), [])

    def test_a_static_or_inner_declaration_does_not(self):
        self.assertEqual(self.names("static int g(int);\nint f(void) {\n    int h(int);\n    return 1;\n}\n"), [])

    def test_the_alias_covers_it(self):
        fake = type("F", (), {"aliases": {"g": {"func_00214D28"}, "func_00214D28": {"g"}}})
        self.assertEqual(me.use_variant("void g(float);\nvoid f(void) { g(1.0f); }\n", fake, "g"),
                         'void g_v(float) __asm__("func_00214D28");\nvoid f(void) { g_v(1.0f); }\n')


class HeaderTest(unittest.TestCase):
    def test_names_leave_and_others_stay(self):
        text = "#ifndef A\nextern int a(int);\nextern int b(void *);\n#endif\n"
        self.assertEqual(me.without_names(text, {"a"}), "#ifndef A\nextern int b(void *);\n#endif\n")


class SplitTest(unittest.TestCase):
    def test_a_shared_statement_splits(self):
        new, count = me.split_shared_statements("extern float a(int), b(float, int);\nint x;\n")
        self.assertEqual((new, count), ("extern float a(int);\nextern float b(float, int);\nint x;\n", 1))

    def test_a_pointer_head_or_a_label_stays(self):
        for text in ('extern int *a(int), b(int);\n', 'extern int a(int), b(int) __asm__("c");\n'):
            self.assertEqual(me.split_shared_statements(text), (text, 0))


class VariantNamesTest(unittest.TestCase):
    def test_a_declaration_inside_a_function_gets_the_label(self):
        fake = type("F", (), {"aliases": {"g": {"func_00214D28"}, "func_00214D28": {"g"}}})
        text = "void f(void) {\n    extern char g[];\n    h(g);\n}\n"
        self.assertEqual(me.use_variant(text, fake, "g"),
                         'void f(void) {\n    extern char g_v[] __asm__("func_00214D28");\n    h(g_v);\n}\n')

    def test_both_spellings_get_one_name(self):
        fake = type("F", (), {"aliases": {"func_00205A50": {"find_id"}, "find_id": {"func_00205A50"}}})
        self.assertEqual(me.variant_names(fake, "func_00205A50"),
                         {"func_00205A50": "find_id_v", "find_id": "find_id_v"})


class OldStyleTest(unittest.TestCase):
    def test_an_old_style_definition_is_found(self):
        self.assertEqual(me.OLD_STYLE.findall("void f(p, q)\n    char *p;\n    int q;\n{\n}\n"), ["f"])

    def test_a_prototype_is_not(self):
        self.assertEqual(me.OLD_STYLE.findall("void f(int p)\n{\n    int q;\n}\nextern void g(p);\n"), [])


if __name__ == "__main__":
    unittest.main()
