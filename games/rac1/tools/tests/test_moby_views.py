import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tidy"))
import moby_views as mv  # noqa: E402


class IsMobyTest(unittest.TestCase):
    def test_a_moby_only_field_is_enough(self):
        self.assertTrue(mv.is_moby("x = *(short *)(m + 0xA6);", "m"))

    def test_the_layout_must_agree(self):
        self.assertIsNone(mv.is_moby("*(int *)(m + 0x228); *(int *)(m + 0x22C); *(short *)(m + 0xA6);", "m"))

    def test_a_vector_alone_is_not_enough(self):
        body = "*(float *)(m + 0x10); *(float *)(m + 0x14); *(float *)(m + 0x18);"
        self.assertIsNone(mv.is_moby(body, "m"))

    def test_three_fields_are_enough(self):
        self.assertTrue(mv.is_moby("*(float *)(m + 0x10); *(unsigned char *)(m + 0x20); *(unsigned short *)(m + 0x34);", "m"))

    def test_the_function_name_helps(self):
        body = "*(float *)(m + 0x84); *(float *)(m + 0x88);"
        self.assertIsNone(mv.is_moby(body, "m", "update_range"))
        self.assertTrue(mv.is_moby(body, "m", "update_moby_range"))

    def test_another_name_needs_more(self):
        one = "*(short *)(t + 0xA6);"
        self.assertIsNone(mv.is_moby(one, "t"))
        self.assertIsNone(mv.is_moby("*(float *)(t + 0x10); *(unsigned char *)(t + 0x20); *(unsigned short *)(t + 0x34);", "t"))
        self.assertTrue(mv.is_moby("*(float *)(t + 0x10); *(float *)(t + 0x14); *(short *)(t + 0xA6);", "t"))

    def test_a_wrong_size_does_not_fit(self):
        self.assertFalse(mv.access_fits("int", "*", 0x34))
        self.assertTrue(mv.access_fits("short", "*", 0x34))
        self.assertTrue(mv.access_fits("char", "**", 0x78))

    def test_a_particle_is_not_a_moby(self):
        self.assertIsNone(mv.is_moby("m = create_part(1); *(short *)(m + 0xA6);", "m"))


class CandidatesTest(unittest.TestCase):
    def test_a_real_moby_param(self):
        text = "void f(char *m) {\n    *(short *)(m + 0xA6) = 1;\n}\n"
        self.assertEqual(dict(mv.candidates(text)), {"m": ["f"]})

    def test_a_typed_param_is_left(self):
        self.assertEqual(dict(mv.candidates("void f(Camera *m) {\n    *(short *)(m + 0xA6) = 1;\n}\n")), {})


if __name__ == "__main__":
    unittest.main()
