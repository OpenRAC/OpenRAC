import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tidy"))
import voice_rows as vr  # noqa: E402

SOURCE = '''#include "common.h"
typedef struct {
    char p0[0x74];
    unsigned char f74;
    char p1[0x13];
    unsigned char *f88;
} Row;
int f(int i) {
    {
        Row *e = (Row *)(voice_pool + i * 0x70);
        return e->f88 == ((unsigned char *)g_hero.moby) && e->f74 != 0;
    }
    return other->f74;
}
'''


class VoiceRowsTest(unittest.TestCase):
    def test_a_view_becomes_the_shared_type(self):
        new, done = vr.convert(SOURCE)
        self.assertEqual(done, {"Row": {"f74": "state", "f88": "owner"}})
        self.assertIn('#include "structs/voice.h"', new)
        self.assertIn("VoicePoolRow *e = (VoicePoolRow *)(voice_pool + i * 0x70);", new)
        self.assertIn("return e->owner == g_hero.moby && e->state != 0;", new)
        self.assertNotIn("typedef", new)

    def test_another_variable_keeps_its_field(self):
        self.assertIn("return other->f74;", vr.convert(SOURCE)[0])

    def test_a_field_with_no_place_stays(self):
        self.assertIsNone(vr.field_map("char p0[0x76]; short odd;"))

    def test_names_and_sizes(self):
        self.assertEqual(vr.field_map("u8 pad0[0x88]; struct Moby *owner; u8 pad8C[4]; Vec4f pos;"),
                         {"owner": "owner", "pos": "position"})
        self.assertEqual(vr.field_map("char p0[0x74]; unsigned char on; char p1[0x88 - 0x75]; Moby *owner;"),
                         {"on": "state", "owner": "owner"})
        self.assertEqual(vr.field_map("u8 pad_00[0x7E]; s16 linked_index; u8 pad_80[0x8]; s32 owner;"),
                         {"linked_index": "linked_index", "owner": "owner"})

    def test_a_pointer_named_like_padding_is_a_field(self):
        self.assertEqual(vr.field_map("char p0[0x74]; unsigned char b74; char p75[0x13]; struct Moby *p88;"),
                         {"b74": "state", "p88": "owner"})

    def test_the_include_goes_above_the_first_use(self):
        self.assertEqual(vr.add_include('#include "a.h"\nVoicePoolRow *x;\n#include "b.h"\n'),
                         '#include "a.h"\n#include "structs/voice.h"\nVoicePoolRow *x;\n#include "b.h"\n')

    def test_a_store_inside_a_test_is_renamed(self):
        self.assertIn("(e = (VoicePoolRow *)(voice_pool + 1 * 0x70))->state;",
                      vr.rename_fields("(e = (Row *)(voice_pool + 1 * 0x70))->f74;", "Row", {"f74": "state"}))

    def test_a_type_with_no_voice_pool_cast_stays(self):
        text = "typedef struct { char p[0x74]; unsigned char a; } T;\nT *t;\n"
        self.assertEqual(vr.convert(text), (text, {}))


if __name__ == "__main__":
    unittest.main()
