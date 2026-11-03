import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tidy"))
import hero_records as hr  # noqa: E402

COPY = ("typedef struct {\n    u8 pad0[0x18];\n    s32 i18;\n    s32 i1C;\n    union {\n        Q q;\n        f32 f[4];\n"
        "    } v20;\n    u8 pad30[0x10];\n    f32 f40;\n    f32 f44;\n} HS;\n")


class HeroRecordsTest(unittest.TestCase):
    def test_a_copy_converts(self):
        text = '#include "a.h"\n' + COPY + "extern HS hit;\nf32 f(void) { return hit.v20.f[2] + hit.f44; }\n"
        new, done = hr.convert(text)
        self.assertEqual(done, ["HS"])
        self.assertEqual(new, '#include "a.h"\n#include "structs/collision.h"\nextern struct CollisionHit hit;\n'
                              "f32 f(void) { return hit.point.f[2] + hit.normal.f[1]; }\n")

    def test_an_alias_counts(self):
        text = COPY + 'extern HS hit_r __asm__("D_00100000");\nint g(void) { return hit_r.i18; }\n'
        self.assertIn("return hit_r.moby;", hr.convert(text)[0])

    def test_another_body_stays(self):
        text = "typedef struct {\n    int a;\n} HS;\nextern HS hit;\n"
        self.assertEqual(hr.convert(text), (text, []))

    def test_another_variable_keeps_its_field(self):
        text = COPY + "extern HS hit;\nint g(void) { return other.i18 + hit.i18; }\n"
        self.assertIn("other.i18 + hit.moby", hr.convert(text)[0])


if __name__ == "__main__":
    unittest.main()
