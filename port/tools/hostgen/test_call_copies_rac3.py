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


class MisnamedCalleeTest(unittest.TestCase):
    """A C call by a name no copy of the caller reaches (another level's function): the copies' own
    jal, when the caller has only the one that differs."""

    def program(self) -> Program:
        program = Program()
        program.places = {"func_L08_004D1D30": [(8, 0x4D1D30), (27, 0x4C6178)],
                          "func_L01_004EF840": [(1, 0x4EF840), (2, 0x4C4D98)],
                          "func_L08_004D0B40": [(8, 0x4D0B40)], "func_L04_004FE9E0": [(27, 0x4C4F10)]}
        program.call_copies = {}
        program.call_copy_callees = {}
        for callee, home, own in (("func_L04_004FE9E0", 27, 0x4C4F10), ("func_L08_004D0B40", 8, 0x4D0B40)):
            program.call_copies[("func_L08_004D1D30", callee)] = (home, 0x4D1D30, 1444, own, 0)
            program.call_copy_callees.setdefault("func_L08_004D1D30", []).append((callee, frozenset({154, 164})))
        return program

    def test_a_name_no_copy_reaches_is_the_copys_call(self):
        self.assertEqual(call_copy(self.program(), "func_L08_004D1D30", "func_L01_004EF840"),
                         (27, 0x4D1D30, 1444, 0x4C4F10, 0))

    def test_not_where_the_copies_differ_at_several_jals(self):
        program = self.program()
        program.call_copy_callees["func_L08_004D1D30"][1] = ("func_L08_004D0B40", frozenset({7}))
        self.assertIsNone(call_copy(program, "func_L08_004D1D30", "func_L01_004EF840"))

    def test_not_a_name_a_copy_has(self):
        program = self.program()
        program.places["func_L01_004EF840"].append((8, 0x4D1000))
        self.assertIsNone(call_copy(program, "func_L08_004D1D30", "func_L01_004EF840"))


if __name__ == "__main__":
    unittest.main()


class FrontEndNamesInLevels(unittest.TestCase):
    """rac3-decomp's level trees write a front-end function as fe_func_XXXXXXXX: in a level's C it
    is that level's copy (tools/level_tree.py). func_L01_003E4198's fe_func_003D3CB0, the camera's
    coll_line, was a stub that did nothing."""

    def test_an_fe_name_is_the_levels_copy(self):
        program = Program()
        program.kind_renames = {"func_003D3CB0": "func_L01_003FA5F0", "func_0038CE20": "func_L01_0040E3C0"}
        unit = "src/levels/shared/L01_003E3930.c"
        self.assertEqual(program.level_copy_named(unit, "fe_func_003D3CB0"), "func_L01_003FA5F0")
        self.assertEqual(program.level_copy_named(unit, "fe_func_0038CE20"), "func_L01_0040E3C0")
        # One the catalogue has no front-end row for: the bare name, which the file's level places.
        self.assertEqual(program.level_copy_named(unit, "fe_func_00443948"), "func_00443948")
        # Not in a front end's own files.
        self.assertIsNone(program.level_copy_named("src/frontbin/3D1030.c", "fe_func_003D3CB0"))


class Vu0AssemblyFromRetail(unittest.TestCase):
    """hostgen.json "vu0_assembly_from_retail": C whose inline assembly runs VU0 instructions on named
    registers (rac3's divide_vector_xyz, func_L01_0040E408) is translated from its assembly: as C it
    kept a barrier and wrote nothing."""

    @staticmethod
    def asm(text: str) -> dict:
        return {"kind": "GCCAsmStmt", "asmString": text}

    def test_the_instructions_of_the_body_and_its_static_helpers(self):
        from program import vu0_assembly
        helper = {"kind": "FunctionDecl", "name": "vec_div", "inner": [
            self.asm("mfc1 $at, %2\nlqc2 $vf1, 0(%1)\nqmtc2.ni $at, $vf2\nvdiv Q, $vf0w, $vf2x\nvwaitq")]}
        body = {"kind": "FunctionDecl", "inner": [
            self.asm("sync.l"),
            {"kind": "CallExpr", "inner": [{"kind": "DeclRefExpr",
                                            "referencedDecl": {"kind": "FunctionDecl", "name": "vec_div"}}]},
            self.asm("vmulq.xyz $vf1, $vf1, Q; sqc2 $vf1, 0(%0)")]}
        self.assertEqual(vu0_assembly(body, {"vec_div": helper}),
                         ("lqc2", "qmtc2.ni", "vdiv", "vwaitq", "vmulq.xyz", "sqc2"))
        self.assertEqual(vu0_assembly({"kind": "FunctionDecl", "inner": [self.asm("sync.l"), self.asm("di")]}, {}),
                         ())

    def test_such_a_definition_is_left_to_its_assembly(self):
        from program import users_of_vu0_assembly
        program = Program()
        program.places = {"func_L01_0040E408": [(1, 0x0040E408)], "func_L01_0040E3C0": [(1, 0x0040E3C0)]}
        sig = Signature.of(ctype.parse("void (int, int, float)"))
        program.functions["func_L01_0040E408"] = Function("func_L01_0040E408", sig, "src/levels/a.c",
                                                          vu0_asm=("lqc2", "vdiv"))
        program.functions["func_L01_0040E3C0"] = Function("func_L01_0040E3C0", sig, "src/levels/a.c")
        self.assertEqual(users_of_vu0_assembly(program), {"func_L01_0040E408": ["lqc2", "vdiv"]})
        self.assertIsNone(program.functions["func_L01_0040E408"].unit)
        self.assertIn("func_L01_0040E408", program.from_assembly)
        self.assertEqual(program.functions["func_L01_0040E3C0"].unit, "src/levels/a.c")

    def test_a_microprogram_call_keeps_its_c(self):
        from program import users_of_vu0_assembly
        program = Program()
        program.places = {"func_L01_003D1DA8": [(1, 0x003D1DA8)]}
        program.functions["func_L01_003D1DA8"] = Function(
            "func_L01_003D1DA8", Signature.of(ctype.parse("void (int)")), "src/levels/a.c", vu0_asm=("vcallms", "qmfc2.i"))
        self.assertEqual(users_of_vu0_assembly(program), {})

    def test_after_a_translation_only_what_it_could_not_write_as_c_moves(self):
        # vu0asm.py writes VU0 statements as C; the second translation (hostgen.py vu0_refusals) moves
        # only the functions whose statements it refused or skipped, a static helper's callers among them.
        from program import users_of_vu0_assembly, vu0_helpers
        helper = {"kind": "FunctionDecl", "name": "vec_div", "inner": [
            {"kind": "GCCAsmStmt", "asmString": "lqc2 $vf1, 0(%1)\nvdiv $Q, $vf1x, $vf1y"}]}
        caller = {"kind": "FunctionDecl", "inner": [
            {"kind": "DeclRefExpr", "referencedDecl": {"kind": "FunctionDecl", "name": "vec_div"}}]}
        self.assertEqual(vu0_helpers(caller, {"vec_div": helper}), ("vec_div",))
        sig = Signature.of(ctype.parse("void (int)"))
        names = ("func_L01_0040E408", "func_L01_0040E3C0", "func_L01_0040E388")
        program = Program()
        program.places = {n: [(1, int(n[-8:], 16))] for n in names}
        program.functions[names[0]] = Function(names[0], sig, "src/levels/a.c", vu0_asm=("lqc2", "vdiv"))
        program.functions[names[1]] = Function(names[1], sig, "src/levels/a.c", vu0_asm=("lqc2", "vdiv"),
                                               vu0_via=("vec_div",))
        program.functions[names[2]] = Function(names[2], sig, "src/levels/a.c", vu0_asm=("lqc2", "vadd.xyz"))
        program.vu0_refused = frozenset()
        self.assertEqual(users_of_vu0_assembly(program), {})
        program.vu0_refused = frozenset({names[0], "vec_div"})
        self.assertEqual(sorted(users_of_vu0_assembly(program)), [names[1], names[0]])
        self.assertEqual(program.functions[names[2]].unit, "src/levels/a.c")


class VoidTailLeftToAssembly(unittest.TestCase):
    """inherit_results runs before the assembly stage. Annihilation Nation's func_L07_003B6E98 ends in
    a call of func_L07_00473C80, whose C returns the effect it made: the caller took that result. Left
    to its assembly (its C's VU0 instructions), the callee is `void` as its callers declare it, and the
    caller returned a void call's value, which did not compile."""

    def test_the_caller_returns_nothing_again(self):
        from program import canon, inherit_results, uninherit_void_results
        program = Program()
        unit = "src/levels/07_annihilation_nation/L07_003B6800.c"
        callee = Function("func_L07_00473C80", Signature.of(ctype.parse("void *(void *, int)")), unit=unit)
        caller = Function("func_L07_003B6E98", Signature.of(ctype.parse("void (void *, int)")), unit=unit,
                          tail="func_L07_00473C80")
        program.functions = {f.symbol: f for f in (callee, caller)}
        self.assertEqual(inherit_results(program), ["func_L07_003B6E98"])
        callee.sig = Signature.of(ctype.parse("void (void *, int)"))  # what the assembly stage found
        self.assertEqual(uninherit_void_results(program), ["func_L07_003B6E98"])
        self.assertEqual(canon(caller.sig.ret, program.typedefs), "void")
        self.assertFalse(caller.inherits)
        self.assertEqual(uninherit_void_results(program), [])
