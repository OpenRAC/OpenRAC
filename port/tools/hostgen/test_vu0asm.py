# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Synthetic VU0 lowering checks, with native C execution on Windows as well as POSIX hosts.

These use host-local arrays and the runtime's inline arithmetic, not the POSIX game-memory harness.
Expected values are small exact arithmetic; no disc data, game function or global vector unit is used.
"""
from pathlib import Path
import os
import shutil
import subprocess
import sys
import tempfile
import unittest

import vu0asm

RUNTIME = Path(__file__).resolve().parents[2] / "runtime/include"
CLANG = os.environ.get("OPENRAC_CLANG") or shutil.which("clang")


def memory(k: int, name: str, output: bool = False) -> vu0asm.Operand:
    """An array operand in the synthetic host program, with no guest-memory dependency."""
    return vu0asm.Operand(k, "=m" if output else "m", output, "arr", "float[4]", lv_text=name)


class UnsupportedState(unittest.TestCase):
    def test_control_registers_without_tracked_semantics_are_refused(self):
        # STATUS, MAC, CLIP, R and I are not tracked by this native inline-asm lowering.
        for register in (16, 17, 18, 20, 21):
            with self.subTest(register=register), self.assertRaises(vu0asm.Vu0Unsupported):
                vu0asm.lower_statement(f"cfc2 %0, ${register}", [
                    vu0asm.Operand(0, "=r", True, lv_text="result")
                ], vu0asm.FunctionState(), vu_flush=False)

    def test_an_untracked_immediate_scalar_is_not_silently_zero(self):
        # I can come from a caller's control-register write; local zero is not its value.
        with self.assertRaises(vu0asm.Vu0Unsupported):
            vu0asm.lower_statement("vaddi.xyzw $vf1, $vf0, I", [],
                                  vu0asm.FunctionState(), vu_flush=False)


@unittest.skipUnless(CLANG, "needs the installed host Clang")
class NativeArithmetic(unittest.TestCase):
    def execute(self, declarations: str, body: str) -> list[float]:
        """Compile and run the emitted C against the production inline arithmetic helpers."""
        with tempfile.TemporaryDirectory(prefix="hostgen vu0 native ") as directory:
            root = Path(directory)
            source = root / "main.c"
            binary = root / ("test.exe" if sys.platform == "win32" else "test")
            source.write_text(
                '#include <stdio.h>\n#include "openrac/ee.h"\n'
                '__extension__ typedef unsigned __int128 openrac_uti;\nint main(void) {\n'
                'openrac_vu0_state state = {{{0.0f, 0.0f, 0.0f, 1.0f}}};\n'
                'openrac_vu0_state *const openrac_vu_ = &state; (void)openrac_vu_;\n'
                + declarations + '\n' + body
                + '\nprintf("%.6f %.6f %.6f %.6f\\n", out[0], out[1], out[2], out[3]);\n'
                'return 0; }\n', encoding="utf-8")
            command = [CLANG, "-std=gnu11", "-O0", "-ffp-contract=off", "-I", str(RUNTIME),
                       str(source), "-o", str(binary)]
            if sys.platform != "win32":
                command.append("-lm")
            built = subprocess.run(command, capture_output=True, text=True)
            self.assertEqual(built.returncode, 0, built.stderr)
            ran = subprocess.run([str(binary)], capture_output=True, text=True, timeout=10)
            self.assertEqual(ran.returncode, 0, ran.stderr)
            return [float(value) for value in ran.stdout.split()]

    def test_vector_operands_add_all_four_components(self):
        body = vu0asm.lower_statement("vadd.xyzw %0, %1, %2", [
            memory(0, "out", True), memory(1, "a"), memory(2, "b")
        ], vu0asm.FunctionState(), vu_flush=False)
        result = self.execute("float out[4] = {0}, a[4] = {1, 2, 3, 9}, b[4] = {0.5f, -1, 0.25f, 7};", body)
        self.assertEqual(result, [1.5, 1.0, 3.25, 16.0])
        self.assertNotIn("OPENRAC_VU0", body)

    def test_named_registers_and_accumulator_carry_between_statements(self):
        state = vu0asm.FunctionState()
        body = vu0asm.lower_statement("lqc2 $vf1, %0\nlqc2 $vf2, %1", [
            memory(0, "a"), memory(1, "b")
        ], state, vu_flush=False)
        body += vu0asm.lower_statement(
            "vmulax.xyzw ACC, $vf1, $vf2x\nvmaddw.xyzw $vf3, $vf0, $vf2w",
            [], state, vu_flush=False)
        body += vu0asm.lower_statement("sqc2 $vf3, %0", [memory(0, "out", True)], state, vu_flush=False)
        result = self.execute("float out[4] = {0}, a[4] = {1, 2, 3, 9}, b[4] = {2, 0, 0, 4};", body)
        # a * b.x + (0, 0, 0, 1) * b.w.
        self.assertEqual(result, [2.0, 4.0, 6.0, 22.0])

    def test_q_division_and_broadcast_use_the_selected_components(self):
        body = vu0asm.lower_statement(
            "lqc2 $vf1, %1\nvdiv Q, $vf0w, $vf1x\nvmulq.xyzw $vf2, $vf1, Q\nsqc2 $vf2, %0",
            [memory(0, "out", True), memory(1, "a")], vu0asm.FunctionState(), vu_flush=False)
        result = self.execute("float out[4] = {0}, a[4] = {4, 8, 12, 16};", body)
        self.assertEqual(result, [1.0, 2.0, 3.0, 4.0])


if __name__ == "__main__":
    unittest.main()
