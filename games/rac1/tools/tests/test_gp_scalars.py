import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tidy"))
import gp_scalars as gp  # noqa: E402

TEXT = "extern short D_L00_00161CA0;\nfloat f(void) { return *(float *)&D_L00_00161CA0; }\n"


class GpScalarTest(unittest.TestCase):
    def test_one_cast_type_converts(self):
        self.assertEqual(gp.candidates(TEXT), {"D_L00_00161CA0": "float"})
        self.assertEqual(gp.apply(TEXT, {"D_L00_00161CA0": "float"}, {}),
                         "extern float D_L00_00161CA0 SDATA(D_L00_00161CA0);\nfloat f(void) { return D_L00_00161CA0; }\n")

    def test_a_readable_name_uses_its_address(self):
        text = "extern short speed;\nfloat f(void) { return *(float *)&speed; }\n"
        self.assertIn("extern float speed SDATA(D_L00_00161CA0);", gp.apply(text, {"speed": "float"}, {"speed": {"D_L00_00161CA0"}}))

    def test_two_cast_types_stay(self):
        text = TEXT + "int g(void) { return *(int *)&D_L00_00161CA0; }\n"
        self.assertEqual(gp.candidates(text), {})

    def test_a_plain_use_stays(self):
        self.assertEqual(gp.candidates(TEXT + "short h(void) { return D_L00_00161CA0; }\n"), {})

    def test_a_fragment_counts(self):
        self.assertEqual(gp.candidates(TEXT, "int v(void) { return *(int *)&D_L00_00161CA0; }\n"), {})
        self.assertEqual(gp.candidates(TEXT, "float v(void) { return *(float *)&D_L00_00161CA0; }\n"),
                         {"D_L00_00161CA0": "float"})

    def test_a_field_of_it_stays(self):
        self.assertEqual(gp.candidates("extern short x;\nfloat f(void) { return (*(float *)&x)[1]; }\n"), {"x": "float"})
        self.assertEqual(gp.candidates("extern short x;\nint a = *(int *)&x[1];\n"), {})

    def test_a_trailing_comment_does_not_hide_the_declaration(self):
        text = "extern short speed; /* SDA, gp -0x78B4 */\nfloat f(void) { return *(float *)&speed; }\n"
        self.assertEqual(gp.apply(text, {"speed": "float"}, {"speed": {"D_00161CA0"}}),
                         "extern float speed SDATA(D_00161CA0);\nfloat f(void) { return speed; }\n")

    def test_the_useful_part_of_a_note_stays(self):
        self.assertEqual(gp.live_note(" /* SDA, gp -0x6DB0: mismatch count */"), " /* mismatch count */")
        self.assertEqual(gp.live_note(" /* declared small so -G2 puts it in SDA */"), "")
        self.assertEqual(gp.live_note(" /* the frame timer */"), " /* the frame timer */")

    def test_every_declaration_converts(self):
        text = "extern short x;\nextern short x;\nfloat f(void) { return *(float *)&x; }\n"
        self.assertEqual(gp.candidates(text), {"x": "float"})

    def test_brackets_go_from_an_increment(self):
        text = "extern short x;\nvoid f(void) { (*(int *)&x)++; }\n"
        self.assertIn("{ x++; }", gp.apply(text, {"x": "int"}, {"x": {"D_00161CA0"}}))

    def test_two_spellings_of_one_type_count(self):
        text = "extern short x;\nint f(void) { return *(int *)&x + *(s32 *)&x; }\n"
        self.assertEqual(gp.candidates(text), {"x": "int"})
        self.assertIn("return x + x;", gp.apply(text, {"x": "int"}, {"x": {"D_00161CA0"}}))

    def test_an_address_counts(self):
        text = "extern short x;\nvoid f(void) { g((int *)&x); h(*(int *)&x); }\n"
        self.assertEqual(gp.candidates(text), {"x": "int"})
        self.assertIn("g(&x); h(x);", gp.apply(text, {"x": "int"}, {"x": {"D_00161CA0"}}))

    def test_an_indexed_address_stays(self):
        self.assertEqual(gp.candidates("extern short x;\nint f(void) { return ((int *)&x)[1]; }\n"), {})

    def test_a_byte_address_alone_stays(self):
        self.assertEqual(gp.candidates("extern short x;\nvoid f(void) { g((char *)&x); }\n"), {})

    def test_a_pointer_global(self):
        text = "extern short x;\nchar *f(void) { return *(char **)&x; }\n"
        self.assertEqual(gp.candidates(text), {"x": "char *"})
        self.assertIn("extern char *x SDATA(D_00161CA0);", gp.apply(text, {"x": "char *"}, {"x": {"D_00161CA0"}}))

    def test_no_address_name_means_no_change(self):
        text = "extern short speed;\nfloat f(void) { return *(float *)&speed; }\n"
        self.assertEqual(gp.apply(text, {"speed": "float"}, {}), text)


if __name__ == "__main__":
    unittest.main()
