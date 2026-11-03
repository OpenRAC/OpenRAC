import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tidy"))
import move_protos as mp  # noqa: E402

HOME = "void f(int a) { }\nvoid g(void) { }\n"
USER = '#include "common.h"\nextern void f(int a);\nvoid u(void) { f(1); }\n'


class PlanTest(unittest.TestCase):
    def plan(self, sources, fragments=None, aliases=None):
        return mp.plan(sources, fragments or {}, aliases or {})

    def test_same_text_in_two_files_moves_to_the_defining_files_header(self):
        headers, takes = self.plan({"src/game/d.c": HOME, "src/game/a.c": USER, "src/game/b.c": USER})
        self.assertEqual(headers, {"game/d.h": {"f": "extern void f(int a);"}})
        self.assertEqual(takes["src/game/a.c"], {"game/d.h": {"f"}})

    def test_one_user_is_not_enough(self):
        headers, _ = self.plan({"src/game/d.c": HOME, "src/game/a.c": USER})
        self.assertEqual(headers, {})

    def test_different_text_stays(self):
        other = "extern int f(int a);\n"
        headers, _ = self.plan({"src/game/d.c": HOME, "src/game/a.c": USER, "src/game/b.c": USER, "src/game/c.c": other})
        self.assertEqual(headers, {})

    def test_a_second_spelling_with_another_type_stays(self):
        aliases = {"f": {"func_1"}, "func_1": {"f"}}
        other = "extern int func_1(void);\n"
        headers, _ = self.plan({"src/game/d.c": HOME, "src/game/a.c": USER, "src/game/b.c": USER, "src/game/c.c": other},
                               aliases=aliases)
        self.assertEqual(headers, {})

    def test_qualifiers_and_types_stay(self):
        self.assertFalse(mp.is_candidate("f", 'extern void f(void) __asm__("func_1");'))
        self.assertFalse(mp.is_candidate("f", "extern void f(struct Moby *m);"))
        self.assertFalse(mp.is_candidate("f", "extern void f(Vec4 *v);"))
        self.assertTrue(mp.is_candidate("x", "extern int x;"))
        self.assertTrue(mp.is_candidate("f", "extern s32 f(void *a, float b);"))

    def test_two_definitions_stay(self):
        headers, _ = self.plan({"src/game/d.c": HOME, "src/game/e.c": HOME, "src/game/a.c": USER, "src/game/b.c": USER})
        self.assertEqual(headers, {})

    def test_a_file_using_another_name_of_the_header_keeps_its_declarations(self):
        both = "extern void g(void);\n"
        clash = "extern void f(int a);\nvoid u(void) { g(); }\n"
        sources = {"src/game/d.c": HOME, "src/game/a.c": USER, "src/game/b.c": USER, "src/game/c.c": clash,
                   "src/game/x.c": both, "src/game/y.c": both}
        _, takes = self.plan(sources)
        self.assertNotIn("src/game/c.c", takes)

    def test_a_name_already_in_the_header_counts_too(self):
        clash = "extern void f(int a);\nint old(void) { return 1; }\n"
        sources = {"src/game/d.c": HOME, "src/game/a.c": USER, "src/game/b.c": USER, "src/game/c.c": clash}
        _, takes = mp.plan(sources, {}, {}, {"game/d.h": {"old"}})
        self.assertNotIn("src/game/c.c", takes)
        self.assertIn("src/game/a.c", takes)

    def test_a_version_fragment_counts_as_using_a_name(self):
        sources = {"src/game/d.c": HOME, "src/game/a.c": USER, "src/game/b.c": USER, "src/game/c.c": USER,
                   "src/game/x.c": "extern void g(void);\n", "src/game/y.c": "extern void g(void);\n"}
        _, takes = self.plan(sources, fragments={"src/game/c.c": "void v(void) { g(); }\n"})
        self.assertNotIn("src/game/c.c", takes)
        self.assertIn("src/game/a.c", takes)


class CanonicalTest(unittest.TestCase):
    def same(self, a, b):
        self.assertEqual(mp.canonical("f", a), mp.canonical("f", b))

    def differ(self, a, b):
        self.assertNotEqual(mp.canonical("f", a), mp.canonical("f", b))

    def test_spelling_and_parameter_names_do_not_count(self):
        self.same("extern s32 f(s32 a);", "extern int f(int);")
        self.same("extern u8 *f(void *dst, f32 x);", "extern unsigned char *f(void *, float);")
        self.same("extern unsigned f(void);", "extern unsigned int f(void);")
        self.same("extern void f(char *);", "extern void f(char * name);")

    def test_real_differences_count(self):
        self.differ("extern void f(void);", "extern void f();")
        self.differ("extern int f(void);", "extern void f(void);")
        self.differ("extern void f(int);", "extern void f(float);")
        self.differ("extern void f(int, float);", "extern void f(float, int);")
        self.differ("extern void f(int *);", "extern void f(int);")
        self.differ("extern void f(short);", "extern void f(int);")
        self.differ("extern void f(unsigned char);", "extern void f(char);")

    def test_data(self):
        self.same("extern s32 f;", "extern int f;")
        self.differ("extern int f;", "extern short f;")
        self.differ("extern int f;", "extern int f[];")


class HomeTest(unittest.TestCase):
    def test_homes(self):
        self.assertEqual(mp.home_of("f", "extern void f(void);", {"src/game/d.c"}, {}), "game/d.h")
        self.assertEqual(mp.home_of("f", "extern void f(void);", set(), {"f": {"func_001"}}), "asm_functions/executable.h")
        self.assertEqual(mp.home_of("x", "extern int x;", set(), {"x": {"D_L07_123"}}), "data/level_07.h")
        self.assertEqual(mp.home_of("x", "extern int x;", set(), {"x": {"D_L00_123"}}), "data/shared_levels.h")
        self.assertIsNone(mp.home_of("f", "extern void f(void);", {"a.c", "b.c"}, {}))


class ApplyTest(unittest.TestCase):
    def test_declaration_goes_and_include_comes_after_the_last_include(self):
        new = mp.apply_to_source(USER, {"game/d.h": {"f"}})
        self.assertEqual(new, '#include "common.h"\n#include "game/d.h"\n\nvoid u(void) { f(1); }\n')

    def test_a_later_include_does_not_move_the_new_one_down(self):
        text = '#include "common.h"\n\nextern void f(int a);\nvoid u(void) { f(1); }\n#include "late.h"\n'
        new = mp.apply_to_source(text, {"game/d.h": {"f"}})
        self.assertLess(new.index('"game/d.h"'), new.index("void u"))

    def test_a_provenance_note_keeps_the_declaration(self):
        text = '#include "common.h"\nextern void f(int a); /* Adapted from Lombyte\'s project */\n'
        self.assertIn("Adapted", mp.apply_to_source(text, {"game/d.h": {"f"}}))

    def test_a_wrapped_prototype_survives_a_merge(self):
        import tempfile
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "d.h"
            path.write_text("extern void old(int a,\n                int b);\n")
            text = mp.merge_header(path, "game/d.h", {"f": "extern void f(int a);"})
        self.assertIn("extern void old(int a, int b);", text)
        self.assertIn("extern void f(int a);", text)

    def test_header_text(self):
        text = mp.header_text("game/d.h", {"f": "extern void f(int a);"})
        self.assertIn("#ifndef GAME_D_H", text)
        self.assertIn("src/game/d.c", text)
        self.assertIn("extern void f(int a);\n", text)


if __name__ == "__main__":
    unittest.main()
