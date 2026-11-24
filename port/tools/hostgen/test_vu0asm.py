# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Synthetic VU0 lowering checks, with native C execution on Windows as well as POSIX hosts.

These use host-local arrays and the runtime's inline arithmetic, not the POSIX game-memory harness.
Expected values are small exact arithmetic; no disc data, game function or global vector unit is used.
"""
from pathlib import Path
import json
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

    def test_a_partial_vector_write_does_not_initialize_other_components(self):
        with self.assertRaises(vu0asm.Vu0Unsupported):
            vu0asm.lower_statement("vmove.x $vf1, $vf0\nvadd.y $vf2, $vf1, $vf0", [],
                                  vu0asm.FunctionState(), vu_flush=False)

    def test_a_partial_accumulator_write_does_not_initialize_other_components(self):
        with self.assertRaises(vu0asm.Vu0Unsupported):
            vu0asm.lower_statement("vaddax.x ACC, $vf0, $vf0x\nvmadd.y $vf1, $vf0, $vf0", [],
                                  vu0asm.FunctionState(), vu_flush=False)

    def test_a_rotation_reads_the_next_source_component(self):
        with self.assertRaises(vu0asm.Vu0Unsupported):
            vu0asm.lower_statement("vmove.x $vf1, $vf0\nvmr32.x $vf2, $vf1", [],
                                  vu0asm.FunctionState(), vu_flush=False)

    def test_separate_masked_writes_can_initialize_the_whole_vector(self):
        state = vu0asm.FunctionState()
        for component in "xyzw":
            vu0asm.lower_statement(f"vmove.{component} $vf1, $vf0", [], state, vu_flush=False)
        self.assertIn("memcpy", vu0asm.lower_statement("sqc2 $vf1, %0", [memory(0, "out", True)],
                                                     state, vu_flush=False))


class PreparedClobbers(unittest.TestCase):
    def test_vector_clobbers_change_only_after_the_operand_sections(self):
        import prep

        source = 'asm("lqc2 $vf1, %0" :: "m"(take ? a : b) : "vf1", "$1", "memory");\n'
        out = prep.prepare(source)
        self.assertNotIn('"vf1"', out)
        self.assertIn('"lqc2 $vf1, %0"', out)
        self.assertIn('"m"(take ? a : b)', out)
        self.assertIn('"$1"', out)
        self.assertEqual(out.count('"memory"'), 2)
        self.assertEqual(out.count("\n"), source.count("\n"))

    def test_non_vector_assembly_keeps_its_clobbers(self):
        import prep

        source = 'void f(void) { asm("abs.s $f0, $f1" ::: "vf1"); }\n'
        self.assertEqual(prep.prepare(source), source)

    def test_comments_cannot_start_a_clobber_list(self):
        import prep

        source = 'asm("lqc2 $vf1, %0" /* : "vf1" */ :: "m"(x) : "vf1");\n'
        expected = source.replace(': "vf1");', ': "memory");')
        self.assertEqual(prep.prepare(source), expected)


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


@unittest.skipUnless(CLANG, "needs the installed host Clang")
class ReportReasons(unittest.TestCase):
    def report(self, source: str) -> dict:
        """Translate a synthetic unit and return its report without a platform-specific runtime."""
        import hostgen

        with tempfile.TemporaryDirectory(prefix="hostgen vu0 reason ") as directory:
            root = Path(directory)
            (root / "src").mkdir()
            (root / "src/main.c").write_text(source, encoding="utf-8")
            config = root / "hostgen.json"
            config.write_text(json.dumps({"name": "test", "sources": ["src"], "includes": []}), encoding="utf-8")
            out = root / "generated"
            self.assertEqual(hostgen.main(["--game", str(config), "--source", str(root),
                                          "--out", str(out), "--jobs", "1", "--quiet"]), 0)
            return json.loads((out / "report.json").read_text(encoding="utf-8"))

    def test_a_skipped_control_read_keeps_its_reason(self):
        # Mnemonics alone cannot explain why a statement still needs its assembly implementation.
        report = self.report(
            'int read_status(void) { int value; __asm__("cfc2 %0, $16" : "=r"(value)); return value; }\n')
        self.assertEqual(report["inline_assembly_skipped"], {"read_status": "cfc2"})
        self.assertEqual(report.get("inline_assembly_unsupported"), {
            "read_status": ["cfc2 of untracked control register 16"]
        })

    def test_vector_clobbers_do_not_hide_a_supported_native_statement(self):
        report = self.report('''
            typedef struct { float x, y, z, w; } V4;
            void doubled(V4 *in, V4 *out) {
                __asm__("lqc2 $vf1, %1\\nvadd.xyzw $vf2, $vf1, $vf1\\nsqc2 $vf2, %0"
                        : "=m"(*out) : "m"(*in) : "vf1", "vf2", "memory");
            }
        ''')
        self.assertEqual(report["stubbed"], {})
        self.assertEqual(report["inline_assembly_skipped"], {})

    def test_a_conditional_write_does_not_initialize_the_following_region(self):
        report = self.report('''
            typedef struct { float x, y, z, w; } V4;
            void conditional(int take, V4 *in, V4 *out) {
                if (take) { __asm__("lqc2 $vf1, %0" : : "m"(*in)); }
                __asm__("sqc2 $vf1, %0" : "=m"(*out));
            }
        ''')
        self.assertIn("conditional", report.get("inline_assembly_unsupported", {}))

    def test_then_state_is_not_available_in_the_else_branch(self):
        report = self.report('''
            typedef struct { float x, y, z, w; } V4;
            void alternate(int take, V4 *in, V4 *out) {
                if (take) { __asm__("lqc2 $vf1, %0" : : "m"(*in)); }
                else { __asm__("sqc2 $vf1, %0" : "=m"(*out)); }
            }
        ''')
        self.assertIn("alternate", report.get("inline_assembly_unsupported", {}))

    def test_named_state_is_not_assumed_to_survive_a_c_call(self):
        report = self.report('''
            typedef struct { float x, y, z, w; } V4;
            extern void other(void);
            void crossing(V4 *in, V4 *out) {
                __asm__("lqc2 $vf1, %0" : : "m"(*in));
                other();
                __asm__("sqc2 $vf1, %0" : "=m"(*out));
            }
        ''')
        self.assertIn("crossing", report.get("inline_assembly_unsupported", {}))


if __name__ == "__main__":
    unittest.main()
