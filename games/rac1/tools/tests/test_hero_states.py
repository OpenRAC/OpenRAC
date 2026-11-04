import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tidy"))
import hero_states as hs  # noqa: E402

TABLE = {0: "HERO_STATE_IDLE", 7: "HERO_STATE_JUMP"}


def rewrite(text):
    return hs.rewrite(text, TABLE)


class HeroStatesTest(unittest.TestCase):
    def test_the_real_table_reads_only_state_ids(self):
        table = hs.names()
        self.assertEqual(table[0], "HERO_STATE_IDLE")
        self.assertEqual(table[7], "HERO_STATE_JUMP")
        self.assertTrue(all(name.startswith("HERO_STATE_") for name in table.values()))

    def test_compare(self):
        self.assertEqual(rewrite("if (g_hero.state.current == 7) x();"),
                         ("if (g_hero.state.current == HERO_STATE_JUMP) x();", 1))
        self.assertEqual(rewrite("if (p->state.prev2 != 0x7) x();"), ("if (p->state.prev2 != HERO_STATE_JUMP) x();", 1))

    def test_assign(self):
        self.assertEqual(rewrite("g_hero.state.current = 7;"), ("g_hero.state.current = HERO_STATE_JUMP;", 1))

    def test_a_call_changes_only_the_first_argument(self):
        self.assertEqual(rewrite("Hero_SetMoveState(7, 0);"), ("Hero_SetMoveState(HERO_STATE_JUMP, 0);", 1))
        self.assertEqual(rewrite("HeroL17_SetMoveState(7);"), ("HeroL17_SetMoveState(HERO_STATE_JUMP);", 1))

    def test_a_number_with_no_name_stays(self):
        text = "if (g_hero.state.current == 99) Hero_SetMoveState(99, 0);"
        self.assertEqual(rewrite(text), (text, 0))

    def test_other_fields_stay(self):
        for text in ("if (g_hero.state.control_mode == 7) x();", "if (m->state == 7) x();", "/* state.current == 7 */",
                     "foo(7, 0);"):
            self.assertEqual(rewrite(text), (text, 0))

    def test_switch(self):
        new, count = rewrite("switch (g_hero.state.current) { case 7: break; case 99: break; }")
        self.assertEqual(count, 1)
        self.assertIn("case HERO_STATE_JUMP:", new)
        self.assertIn("case 99:", new)

    def test_another_switch_stays(self):
        text = "switch (g_hero.state.control_mode) { case 7: break; }"
        self.assertEqual(rewrite(text), (text, 0))


if __name__ == "__main__":
    unittest.main()
