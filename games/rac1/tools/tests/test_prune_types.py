import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tidy"))
import prune_types as pt  # noqa: E402


def prune(text, fragments="", macros=None):
    return pt.prune(text, fragments, macros or set())


class PruneTypesTest(unittest.TestCase):
    def test_unused_typedef_goes(self):
        new, removed, _ = prune("typedef struct { int a; } T;\nint x;\n")
        self.assertEqual(removed, [["T"]])
        self.assertEqual(new, "\nint x;\n")

    def test_used_typedef_stays(self):
        _, removed, _ = prune("typedef struct { int a; } T;\nT x;\n")
        self.assertEqual(removed, [])

    def test_tag_use_keeps_it(self):
        _, removed, _ = prune("struct S { int a; };\nstruct S *p;\n")
        self.assertEqual(removed, [])

    def test_a_struct_defining_a_variable_stays(self):
        _, removed, _ = prune("struct { int a; } g;\nstruct S { int a; } h;\n")
        self.assertEqual(removed, [])

    def test_type_used_only_by_a_dead_type_goes_too(self):
        _, removed, _ = prune("typedef struct { int a; } A;\ntypedef struct { A a; } B;\n")
        self.assertEqual(sorted(removed), [["A"], ["B"]])

    def test_type_used_by_a_live_type_stays(self):
        _, removed, _ = prune("typedef struct { int a; } A;\ntypedef struct { A a; } B;\nB x;\n")
        self.assertEqual(removed, [])

    def test_version_fragment_and_header_macro_are_uses(self):
        self.assertEqual(prune("typedef struct { int a; } T;\n", fragments="T v;\n")[1], [])
        self.assertEqual(prune("typedef struct { int a; } T;\n", macros={"T"})[1], [])

    def test_a_comment_is_not_a_use(self):
        _, removed, _ = prune("typedef struct { int a; } T;\n/* T is here */\n")
        self.assertEqual(removed, [["T"]])

    def test_sizeof_is_a_use(self):
        _, removed, _ = prune("typedef struct { int a; } T;\nint n = sizeof(T);\n")
        self.assertEqual(removed, [])

    def test_a_struct_inside_a_function_is_left_alone(self):
        _, removed, _ = prune("void f(void) {\n    struct S { int a; };\n}\n")
        self.assertEqual(removed, [])

    def test_a_provenance_note_keeps_it(self):
        _, removed, kept = prune("typedef struct { int a; /* Adapted from Lombyte's project */ } T;\n")
        self.assertEqual((removed, kept), ([], 1))

    def test_an_aligned_typedef_is_named_right(self):
        found = pt.find_types("typedef struct { float f[4]; } __attribute__((aligned(16))) V;\n")
        self.assertEqual(found[0][2], {"V"})


if __name__ == "__main__":
    unittest.main()
