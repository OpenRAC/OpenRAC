# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Tests for call_copies_rac3.py: the jals that differ between a folded function's copies.

    python3 -m unittest port/tools/hostgen/test_call_copies_rac3.py
"""
from __future__ import annotations

import struct
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent))

import call_copies_rac3  # noqa: E402
import ctype  # noqa: E402
from program import Function, Program, Signature, call_copy  # noqa: E402

NOP, JR_RA = 0, 0x03E00008


def jal(target: int) -> int:
    return 0x0C000000 | (target >> 2)


def code(*words: int) -> bytes:
    """A function of 80 bytes (past the wrappers' 64) starting with these words."""
    words = words + (NOP,) * (20 - len(words))
    return struct.pack(f"<{len(words)}I", *words)


CATALOGUE = "\n".join([
    "# name\tkind\tsize\tfingerprint\tremnant\tlevels\tplaces",
    # The emitter's tick: level 3's copy calls the +0x20 maker, the vid-comics' the +0x30 one.
    "func_L03_0034B248\tshared\t80\t0\t0\t3\t03:00001000,30:00002000,39:00003000",
    "func_L03_00461E88\tshared\t8\t0\t0\t2\t03:00001100,30:00002200",
    "func_L30_00442950\tshared\t8\t0\t0\t1\t30:00002100",
    "func_L39_00500000\tlevel\t8\t0\t0\t1\t39:00003100",
    # A front end function: its C runs in the front end too, so it is left out.
    "func_003D1B68\tfrontbin\t80\t0\t0\t2\t03:00001200,30:00002300",
    # Every copy calls the same function: nothing to list.
    "func_L03_00000010\tshared\t80\t0\t0\t2\t03:00001300,30:00002400",
    # A wrapper: one shape folds different routines, so its name keeps its callee.
    "func_L01_004127C0\tshared\t28\t0\t0\t2\t03:00001400,30:00002500",
]) + "\n"

PROGRAMS = {
    3: [(0x1000, code(NOP, jal(0x1100), NOP, JR_RA)), (0x1100, code(JR_RA, NOP)),
        (0x1200, code(jal(0x1100), NOP, JR_RA, NOP)), (0x1300, code(jal(0x1100), NOP, JR_RA, NOP)),
        (0x1400, code(jal(0x1100), NOP, JR_RA, NOP))],
    30: [(0x2000, code(NOP, jal(0x2100), NOP, JR_RA)), (0x2100, code(JR_RA, NOP)), (0x2200, code(JR_RA, NOP)),
         (0x2300, code(jal(0x2100), NOP, JR_RA, NOP)), (0x2400, code(jal(0x2200), NOP, JR_RA, NOP)),
         (0x2500, code(jal(0x2100), NOP, JR_RA, NOP))],
    39: [(0x3000, code(NOP, jal(0x3100), NOP, JR_RA)), (0x3100, code(JR_RA, NOP))],
}


class CallCopiesTest(unittest.TestCase):
    def rows(self):
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp)
            (source / "config/levels").mkdir(parents=True)
            (source / "config/levels/functions.tsv").write_text(CATALOGUE)
            with mock.patch.object(call_copies_rac3, "level_programs", return_value=PROGRAMS):
                return call_copies_rac3.rows(source)

    def test_each_callee_gets_the_place_of_a_copy_calling_it(self):
        rows = [r for r in self.rows() if r[0] == "func_L03_0034B248"]
        self.assertEqual(rows, [
            ("func_L03_0034B248", "func_L03_00461E88", 3, 0x1000, 80, 0x1100, (1,)),
            ("func_L03_0034B248", "func_L30_00442950", 30, 0x2000, 80, 0x2100, (1,)),
        ])  # the multiplayer level's callee is another executable's: left out

    def test_front_end_functions_wrappers_and_calls_that_agree_are_left_out(self):
        callers = {r[0] for r in self.rows()}
        self.assertNotIn("func_003D1B68", callers)
        self.assertNotIn("func_L01_004127C0", callers)
        self.assertNotIn("func_L03_00000010", callers)


class SameShapeTest(unittest.TestCase):
    """hostgen.json "call_copies_same_shape": a C call reaches its copy's own callee only where that
    callee reads no register the named callee's call leaves unset."""

    def program(self) -> Program:
        program = Program()
        for name, sig in (("func_maker", "void (void *, void *, int, int)"),
                          ("func_maker30", "int (int, int, int, int)"),
                          ("func_spawn", "int (int, int, float, int, int)"),
                          ("func_spawn3", "void (int, int, int, int, float, float)")):
            program.functions[name] = Function(name, Signature.of(ctype.parse(sig)))
        program.call_copies = {}
        program.call_copy_callees = {}
        for caller, callee in (("func_tick", "func_maker"), ("func_tick", "func_maker30"),
                               ("func_update", "func_spawn"), ("func_update", "func_spawn3")):
            program.call_copies[(caller, callee)] = (3, 0x1000, 80, 0x1100, 0)
            program.call_copy_callees.setdefault(caller, []).append((callee, frozenset({1})))
        program.call_copies_same_shape = True
        return program

    def test_a_callee_of_the_same_registers_is_the_copys(self):
        self.assertIsNotNone(call_copy(self.program(), "func_tick", "func_maker"))

    def test_a_callee_reading_more_floats_keeps_the_named_one(self):
        self.assertIsNone(call_copy(self.program(), "func_update", "func_spawn"))

    def test_callees_at_another_jal_do_not_count(self):
        program = self.program()
        program.call_copy_callees["func_update"] = [("func_spawn", frozenset({1})), ("func_spawn3", frozenset({7}))]
        self.assertIsNotNone(call_copy(program, "func_update", "func_spawn"))

    def test_assembly_translations_hand_every_register_on(self):
        self.assertIsNotNone(call_copy(self.program(), "func_update", "func_spawn", exe_target=0x1100))


if __name__ == "__main__":
    unittest.main()
