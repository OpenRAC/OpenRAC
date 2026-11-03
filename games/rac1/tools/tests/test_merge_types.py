import re
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tidy"))
import merge_types as mt  # noqa: E402


def size_and_fields(body):
    """Size and {field: (offset, element size)} of a body of char arrays, ints and floats."""
    offset, fields = 0, {}
    body = re.sub(r"\b(int|float) (\w+), (\w+);", r"\1 \2; \1 \3;", body)
    for field in body.split(";"):
        m = re.fullmatch(r"\s*(char|int|float)\s+(\w+)(?:\[(\w+)\])?\s*", field)
        if not m:
            continue
        element = 1 if m.group(1) == "char" else 4
        count = int(m.group(3), 0) if m.group(3) else 1
        if not m.group(2).startswith("p"):
            fields[m.group(2)] = (offset, m.group(1))
        offset += element * count
    return offset, fields


class MergeTableTest(unittest.TestCase):
    def test_every_body_fits_its_definition(self):
        """Same size, and every field at the same offset with the same type."""
        for merge in mt.MERGES:
            size, fields = size_and_fields(merge["definition"])
            for body in merge["bodies"]:
                with self.subTest(merge["new"], body=body):
                    other_size, other_fields = size_and_fields(body)
                    self.assertEqual(other_size, size)
                    for name, place in other_fields.items():
                        self.assertEqual(fields[name], place)

    def test_field_names_say_their_offset(self):
        for merge in mt.MERGES:
            _, fields = size_and_fields(merge["definition"])
            for name, (offset, _) in fields.items():
                if re.fullmatch(r"f[0-9A-F]+", name):
                    self.assertEqual(int(name[1:], 16), offset, f"{merge['new']}.{name}")


class ApplyTest(unittest.TestCase):
    REC = "typedef struct {\n    char p0[0x44]; int f44; int f48; char p1[0x10]; float f5C; int f60; char p2[0xC];\n} Rec70;\n"

    def test_listed_body_is_replaced_and_renamed(self):
        text = '#include "common.h"\n' + self.REC + "extern Rec70 idle_fidgets[];\nRec70 *rec;\n"
        new, renames, headers = mt.apply_to_source(text)
        self.assertEqual(renames, {"Rec70": "HeroIdleFidget"})
        self.assertEqual(headers, {mt.HERO})
        self.assertNotIn("Rec70", new)
        self.assertIn('#include "structs/hero_views.h"', new)
        self.assertIn("extern HeroIdleFidget idle_fidgets[];", new)

    def test_another_body_under_that_name_stays(self):
        text = "typedef struct { int other; } Rec70;\nRec70 x;\n"
        self.assertEqual(mt.apply_to_source(text), (text, {}, set()))

    def test_a_field_with_the_old_name_stays(self):
        self.assertEqual(mt.rename_code("a.Cam = b->Cam; Cam c;", {"Cam": "New"}), "a.Cam = b->Cam; New c;")

    def test_comments_and_strings_stay(self):
        self.assertEqual(mt.rename_code('/* Cam */ char *s = "Cam"; Cam c;', {"Cam": "New"}),
                         '/* Cam */ char *s = "Cam"; New c;')

    def test_a_provenance_note_keeps_the_local_type(self):
        text = "typedef struct {\n    char p0[0x44]; int f44; int f48; char p1[0x10]; float f5C; int f60; char p2[0xC];\n} Rec70; /* Adapted from Lombyte's project */\n"
        new, renames, _ = mt.apply_to_source(text)
        self.assertEqual(renames, {})
        self.assertIn("Adapted", new)

    def test_header_text(self):
        merge = next(m for m in mt.MERGES if m["new"] == "HeroIdleFidget")
        text = mt.header_text(mt.HERO, [merge])
        self.assertIn("#ifndef STRUCTS_HERO_VIEWS_H", text)
        self.assertIn("} HeroIdleFidget;", text)
        self.assertIn("    int f44;\n", text)


if __name__ == "__main__":
    unittest.main()
