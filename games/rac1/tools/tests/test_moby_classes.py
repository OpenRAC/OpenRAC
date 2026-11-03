import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tidy"))
import moby_classes as mc  # noqa: E402

TABLE = {0x3EF: "tesla_claw", 564: "screamer", 92: "gate", 353: "gate"}
NAMES = {0x3EF: "MOBY_CLASS_TESLA_CLAW", 564: "MOBY_CLASS_SCREAMER"}


class NamesTest(unittest.TestCase):
    def test_shared_names_get_their_number(self):
        names = mc.enum_names({92: "gate", 353: "gate", 7: "bolt"}, {})
        self.assertEqual(names, {92: "MOBY_CLASS_GATE_92", 353: "MOBY_CLASS_GATE_353", 7: "MOBY_CLASS_BOLT"})

    def test_kept_names_stay(self):
        self.assertEqual(mc.enum_names({13: "bolt"}, {13: "BOLT_CLASS_ONE"}), {13: "BOLT_CLASS_ONE"})

    def test_display_name(self):
        self.assertEqual(mc.display_name("white_flower"), "White Flower")


class CodeTest(unittest.TestCase):
    def rewrite(self, text):
        return mc.rewrite_code(text, NAMES)

    def test_compare_in_hex_and_decimal(self):
        self.assertEqual(self.rewrite("if (m->oclass == 0x3EF) x();")[0], "if (m->oclass == MOBY_CLASS_TESLA_CLAW) x();")
        self.assertEqual(self.rewrite("if (m->oclass != 564) x();")[0], "if (m->oclass != MOBY_CLASS_SCREAMER) x();")

    def test_a_number_with_no_name_stays(self):
        self.assertEqual(self.rewrite("if (m->oclass == 9) x();"), ("if (m->oclass == 9) x();", 0))

    def test_other_fields_stay(self):
        text = "if (m->state == 0x3EF) x();"
        self.assertEqual(self.rewrite(text), (text, 0))

    def test_switch_cases_at_its_own_level_only(self):
        text = "switch (m->oclass) { case 0x3EF: switch (y) { case 564: break; } break; case 564: break; }"
        new, count = self.rewrite(text)
        self.assertEqual(count, 2)
        self.assertIn("case MOBY_CLASS_TESLA_CLAW:", new)
        self.assertIn("switch (y) { case 564: break; }", new)
        self.assertIn("break; case MOBY_CLASS_SCREAMER: break; }", new)

    def test_another_switch_stays(self):
        text = "switch (m->state) { case 564: break; }"
        self.assertEqual(self.rewrite(text), (text, 0))

    def test_comments_and_strings_stay(self):
        text = '/* oclass == 564 */ char *s = "oclass == 564";'
        self.assertEqual(self.rewrite(text), (text, 0))


class CommentTest(unittest.TestCase):
    def rewrite(self, text):
        return mc.rewrite_comment(text, TABLE)

    def test_forms(self):
        cases = {
            "/* Spawns a moby of class 0x3EF. */": "/* Spawns a Tesla Claw moby. */",
            "/* Spawns the replacement moby (class 564). */": "/* Spawns the replacement moby (Screamer). */",
            "/* The linked class 0x3EF moby's value. */": "/* The linked Tesla Claw moby's value. */",
            "/* Spawns class 564. */": "/* Spawns Screamer. */",
            "/* Mark every moby of class 564. */": "/* Mark every Screamer moby. */",
        }
        for old, new in cases.items():
            with self.subTest(old):
                self.assertEqual(self.rewrite(old), new)

    def test_a_number_with_no_name_stays(self):
        self.assertEqual(self.rewrite("/* class 0x999 */"), "/* class 0x999 */")

    def test_a_name_already_there_is_not_repeated(self):
        self.assertEqual(self.rewrite("/* Screamer (class 564) update. */"), "/* Screamer update. */")
        self.assertEqual(self.rewrite("/* Tesla claw (class 0x3EF). */"), "/* Tesla claw. */")

    def test_provenance_stays(self):
        old = "/* Spawns class 564. Adapted from Lombyte's project (GPL-3.0): class 564 file. */"
        self.assertEqual(self.rewrite(old), "/* Spawns Screamer. Adapted from Lombyte's project (GPL-3.0): class 564 file. */")


if __name__ == "__main__":
    unittest.main()
