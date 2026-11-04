import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tidy"))
import particle_views as pv  # noqa: E402


class ParticleTest(unittest.TestCase):
    def test_a_new_particle(self):
        text = "void f(void) {\n    char *r = create_part(1);\n    *(int *)(r + 4) = 1;\n}\n"
        self.assertEqual(dict(pv.candidates(text)), {"r": ["f"]})

    def test_a_second_value_rules_it_out(self):
        self.assertIsNone(pv.is_particle("r = create_part(1); *(int *)(r + 4) = 1; r = other;", "r"))

    def test_only_type_data_is_not_enough(self):
        self.assertIsNone(pv.is_particle("r = create_part(1); *(int *)(r + 0x20) = 1;", "r"))

    def test_the_header_goes_after_common(self):
        self.assertEqual(pv.add_header('#include "common.h"\nint x;\n'),
                         '#include "common.h"\n#include "structs/particle.h"\nint x;\n')
        self.assertIsNone(pv.add_header("int x;\n"))


if __name__ == "__main__":
    unittest.main()
