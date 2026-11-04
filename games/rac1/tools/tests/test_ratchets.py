import re
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import ratchets  # noqa: E402


def count(metric, text):
    return len(re.findall(ratchets.METRICS[metric][1], text, re.M))


class MetricTest(unittest.TestCase):
    def test_raw_offset_casts(self):
        self.assertEqual(count("raw_offset_casts", "x = *(int *)(slot + 0x24);"), 1)
        self.assertEqual(count("raw_offset_casts", "x = *(volatile u32 *)(p->next + 0x8);"), 1)
        self.assertEqual(count("raw_offset_casts", "x = slot->unk_24;"), 0)

    def test_ids_in_comments(self):
        self.assertEqual(count("ids_in_comments", "/* Spawns Moby_564 */"), 1)
        self.assertEqual(count("ids_in_comments", "/* a moby of class 0x3EF */"), 1)
        self.assertEqual(count("ids_in_comments", "/* Spawns a Screamer */"), 0)

    def test_comments_and_code_are_apart(self):
        text = "int a; /* class 0x3EF */\nint b; // Moby_1\n"
        self.assertEqual(ratchets.COMMENT.findall(text), ["/* class 0x3EF */", "// Moby_1"])
        self.assertNotIn("class", ratchets.COMMENT.sub(" ", text))

    def test_asm_aliases_ignore_line_breaks(self):
        self.assertEqual(count("asm_aliases", 'extern int a __asm__("D_1");'), 1)
        self.assertEqual(count("asm_aliases", 'extern int a\n    __asm__("D_1");'), 1)
        self.assertEqual(count("asm_aliases", 'extern int a;\n__asm__(".section .text");'), 0)

    def test_address_names(self):
        self.assertEqual(count("address_names_in_code", "func_001F4E08(D_L07_0020CB00);"), 2)
        self.assertEqual(count("address_names_in_code", "hero_set_move_state(g_x);"), 0)


if __name__ == "__main__":
    unittest.main()
