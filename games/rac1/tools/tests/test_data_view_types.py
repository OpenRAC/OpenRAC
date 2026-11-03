import sys
import unittest
from collections import Counter
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tidy"))
import data_view_types as dv  # noqa: E402


class ChosenTypeTest(unittest.TestCase):
    def test_one_type(self):
        self.assertEqual(dv.chosen_type(Counter({"f32": 3})), "f32")

    def test_char_says_nothing(self):
        self.assertEqual(dv.chosen_type(Counter({"char": 5, "Camera": 1})), "Camera")
        self.assertIsNone(dv.chosen_type(Counter({"char": 5})))

    def test_two_views_disagree(self):
        self.assertIsNone(dv.chosen_type(Counter({"f32": 1, "Camera": 1})))

    def test_a_header_is_found_by_either_spelling(self):
        headers = {"a.h": "typedef struct { int x; } Thing;", "b.h": "struct Other { int y; };"}
        self.assertEqual(dv.header_of("Thing", headers), "a.h")
        self.assertEqual(dv.header_of("struct Other", headers), "b.h")
        self.assertIsNone(dv.header_of("Missing", headers))


if __name__ == "__main__":
    unittest.main()
