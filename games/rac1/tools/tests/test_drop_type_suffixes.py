import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tidy"))
import drop_type_suffixes as dt  # noqa: E402


class ShortNameTest(unittest.TestCase):
    def test_suffixes_go(self):
        self.assertEqual(dt.short_name("Vars_A7078"), "Vars")
        self.assertEqual(dt.short_name("Hist_266e80_67CC8"), "Hist")
        self.assertEqual(dt.short_name("FsAa_FAB40"), "FsAa")

    def test_other_names_stay(self):
        self.assertIsNone(dt.short_name("Vars"))
        self.assertIsNone(dt.short_name("Rec_12"))
        self.assertIsNone(dt.short_name("Moby_Decade"))


class RenameTest(unittest.TestCase):
    def test_comments_follow_but_provenance_stays(self):
        text = "/* A Vars_A7078. Adapted from Lombyte's project: Vars_A7078. */\nVars_A7078 *v;\n"
        self.assertEqual(dt.rename_all(text, {"Vars_A7078": "Vars"}),
                         "/* A Vars. Adapted from Lombyte's project: Vars_A7078. */\nVars *v;\n")


class PlanTest(unittest.TestCase):
    def test_a_free_name_is_taken(self):
        text = "typedef struct { int a; } Vars_A7078;\nVars_A7078 *v;\n"
        self.assertEqual(dt.plan(text, "", set()), {"Vars_A7078": "Vars"})

    def test_a_word_in_the_file_blocks_it(self):
        text = "typedef struct { int a; } Vars_A7078;\nint Vars;\n"
        self.assertEqual(dt.plan(text, "", set()), {})

    def test_a_fragment_or_a_header_blocks_it(self):
        text = "typedef struct { int a; } Vars_A7078;\n"
        self.assertEqual(dt.plan(text, "int Vars;", set()), {})
        self.assertEqual(dt.plan(text, "", {"Vars"}), {})

    def test_two_types_cannot_share_a_name(self):
        text = "typedef struct { int a; } Vars_A7078;\ntypedef struct { int b; } Vars_B7078;\n"
        self.assertEqual(dt.plan(text, "", set()), {"Vars_A7078": "Vars"})


if __name__ == "__main__":
    unittest.main()
