# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Tests for jumptables.py: a table's addresses matched with the C's labels.

    python3 -m unittest port/tools/hostgen/test_jumptables.py      (no Clang needed)
"""
from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import jumptables  # noqa: E402

JR_RA = 0x03E00008
NOP = 0


def jal(target: int) -> int:
    return (3 << 26) | ((target >> 2) & 0x3FFFFFF)


def li_v0(value: int) -> int:
    return (9 << 26) | (2 << 16) | (value & 0xFFFF)


class Code:
    """A Retail stand-in: words by address, in one program."""

    def __init__(self, words: dict[int, int]):
        self.words = words

    def word(self, level: int, addr: int) -> int | None:
        return self.words.get(addr)


def place(at: int, *words: int) -> dict[int, int]:
    return {at + 4 * i: w for i, w in enumerate(words)}


class Leads(unittest.TestCase):
    def test_first_two_calls(self):
        names = {(2, 0x1000): "PlayMobySound", (2, 0x2000): "PickAmmoDropItem"}
        code = Code(place(0x100, jal(0x1000), NOP, jal(0x2000), NOP, JR_RA, NOP))
        self.assertEqual(jumptables.asm_lead(code, 2, 0x100, names), ["call", "PlayMobySound", "PickAmmoDropItem"])

    def test_returned_constant(self):
        code = Code(place(0x100, JR_RA, li_v0(0x2F66)))
        self.assertEqual(jumptables.asm_lead(code, 2, 0x100, {}), ["return", 0x2F66])


class Align(unittest.TestCase):
    def test_labels_numbered_from_one(self):
        # The Maktar slot machine's prize (slot_machine_award_prize): five cases c1..c5 for indices 0..4,
        # and c3, c4 and c5 all play the same sound first.
        addrs = [0x10, 0x20, 0x30, 0x40, 0x50]
        leads = {0x10: ["call", "PlayMobySound", "part_spawn_bolt_pickups"],
                 0x20: ["return"],
                 0x30: ["call", "PlayMobySound", "PickAmmoDropItem"],
                 0x40: ["call", "PlayMobySound", "moby_create_effect_at_position"],
                 0x50: ["call", "PlayMobySound", "snd_play_from_global_bank"]}
        labels = [["c1", leads[0x10]], ["c2", ["return"]], ["c3", leads[0x30]], ["c4", leads[0x40]],
                  ["c5", leads[0x50]], ["other", ["call", "PlayMobySound"]], ["done", ["return"]]]
        indices = {a: {i} for i, a in enumerate(addrs)}
        got = jumptables.align(addrs, leads, labels, indices)
        self.assertEqual([got[a] for a in addrs], ["c1", "c2", "c3", "c4", "c5"])

    def test_returned_constants_out_of_order(self):
        # Joba's challenge titles (arena_get_challenge_title_text_id): the table is not in address order.
        values = [0x2F66, 0x2F67, 0x2F70, 0x2F71, 0x3243]
        addrs = [0x10, 0x18, 0x20, 0x28, 0x30]
        leads = {a: ["return", v] for a, v in zip(addrs, values)}
        labels = [[f"c{i}", ["return", v]] for i, v in enumerate(values)] + [["def", ["return", 0x2F66]]]
        indices = {0x10: {0}, 0x18: {1}, 0x20: {3}, 0x28: {4}, 0x30: {2}}
        got = jumptables.align(addrs, leads, labels, indices)
        self.assertEqual([got[a] for a in addrs], ["c0", "c1", "c2", "c3", "c4"])


if __name__ == "__main__":
    unittest.main()
