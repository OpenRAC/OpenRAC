"""Allocator dump parsing and isolated compilation contracts; no compiler/assets required."""
import json
import io
import sys
import tempfile
import unittest
from fractions import Fraction
from pathlib import Path, PurePosixPath
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import regalloc


class ParserTests(unittest.TestCase):
    def test_function_sections_do_not_leak(self):
        text = ";; Function func_00000001\na\n;; Function func_000000010\nb\n;; Function other\nc\n"
        self.assertIn("\na\n", regalloc.section(text, "func_00000001"))
        self.assertNotIn("\nb\n", regalloc.section(text, "func_00000001"))
        self.assertEqual(regalloc.section(text, "absent"), "")

    def test_dump_fields_order_assignment_conflicts_and_call_crossing(self):
        local = """;; Function func_00000001
Register 85 used 6 times across 24 insns; user var; crosses 2 calls; pointer.
Register 86 used 2 times across 5 insns in block 2; 4 bytes; GR_REGS or none.
;; Register 86 in 3.
"""
        global_ = """;; Function func_00000001
Pass 1 registers to be allocated in sorted order:
Register 86, refs = 4, live_length = 3, size = 1
Register 85, refs = 6, live_length = 24, size = 1

;; 1 regs to allocate: 85
;; 85 conflicts: 85 86 2 3
;; 86 conflicts:
;; Register dispositions:
85 in 17  86 in 3
;; Hard regs used: 3 17
;; Function unrelated
Register 99, refs = 100, live_length = 1, size = 1
;; 1 regs to allocate: 99
"""
        result = regalloc.parse_dumps("func_00000001", local, global_)
        self.assertEqual(result["allocation_orders"], [[85]])
        self.assertEqual([x["pseudo"] for x in result["candidate_passes"][0]["sorted_candidates"]], [86, 85])
        a, b = result["pseudos"]
        self.assertEqual((a["refs"], a["live_length"], a["calls_crossed"], a["hard_register"]), (6, 24, 2, 17))
        self.assertEqual(a["priority_inferred"], {"numerator": 1, "denominator": 2})
        self.assertEqual(a["conflicts"], [85, 86, 2, 3])
        self.assertEqual(b["stats_source"], "greg pass 1")
        self.assertEqual(b["priority_inferred"], {"numerator": 8, "denominator": 3})
        self.assertIsNone(b["calls_crossed"])
        self.assertEqual(b["conflicts"], [])

    def test_missing_data_is_not_fabricated(self):
        result = regalloc.parse_dumps("f", ";; Function different\n", "")
        self.assertEqual(result["status"], "unavailable")
        self.assertEqual(result["pseudos"], [])
        self.assertTrue(result["unavailable"])
        result = regalloc.parse_dumps("f", ";; Function f\n;; Register 90 in 5.\n", "")
        row = result["pseudos"][0]
        self.assertIsNone(row["refs"])
        self.assertIsNone(row["priority_inferred"])
        self.assertEqual(row["assignment_source"], "lreg local assignment")

    def test_invalid_and_large_priority_inputs(self):
        for refs, live in [(None, 5), (4, None), (4, 0), (4, -1), (0, 4), (-2, 4)]:
            self.assertIsNone(regalloc.priority(refs, live))
        self.assertEqual(regalloc.priority(1, 1), 0)
        refs = (1 << 60) - 1
        self.assertEqual(regalloc.priority(refs, 7), Fraction(59 * refs, 7))

    def test_negative_live_and_empty_actual_queue(self):
        result = regalloc.parse_dumps("f", ";; Function f\nRegister 85 used 4 times across -1 insns.\n",
                                      ";; Function f\n;; 0 regs to allocate: \n")
        self.assertEqual(result["allocation_orders"], [[]])
        self.assertIsNone(result["pseudos"][0]["priority_inferred"])


class CompilationTests(unittest.TestCase):
    @patch.object(regalloc.build, "ROOT", PurePosixPath("/repo"))
    @patch.object(regalloc.build, "COMPILER", PurePosixPath("/repo/build/compiler/tools"))
    def test_recipe_preserves_profile_and_assembler_forwarding(self):
        source = regalloc.build.ROOT / "src/game/example.c"
        work = regalloc.build.ROOT / "build/regalloc/example/run"
        text = regalloc.compile_script(source, work, "-O2 -G32 -Wa,-G32 -frac2-qcopy", ("-dl", "-dg"))
        self.assertIn("-O2 -G32 -frac2-qcopy -dl -dg x.i -o x.s", text)
        self.assertIn("/as -G32 -o x.o x.s", text)
        self.assertLess(text.index(str(regalloc.build.SDATA_ALIASES)), text.index("/as -G32"))
        self.assertIn(str(source.parent), text)
        self.assertNotIn("build/obj", text)
        self.assertNotIn(".inputs", text)
        self.assertNotIn("-Wa,", text)
        self.assertNotIn("-dl", regalloc.compile_script(source, work, "-O2", ()))

    @patch.object(regalloc.build, "ROOT", PurePosixPath("/repo"))
    @patch.object(regalloc.build, "COMPILER", PurePosixPath("/repo/build/compiler/tools"))
    @patch.object(regalloc.build, "SDK_COMPILER", PurePosixPath("/repo/build/compiler/sdk"))
    def test_sdk_recipe(self):
        text = regalloc.compile_script(regalloc.build.ROOT / "src/core/f.c", PurePosixPath("/scratch"),
                                       "sdk -O0 -G2 -Wa,-G2", ("-dl",))
        self.assertIn("/xgcc", text)
        self.assertIn("-S -O0 -G2 -dl", text)
        self.assertIn("/as --r5900-pool-li-d -G2", text)
        self.assertLess(text.index(str(regalloc.build.SDATA_ALIASES)), text.index("/as --r5900"))
        with self.assertRaises(ValueError):
            regalloc.split_flags("-O2 -Wa,-G2,")

    def test_version_specific_names_reach_catalogue_lookup(self):
        for name in ("func_PAL_00000001", "func_PAL_L04_00000001", "func_US2_L22_00000001"):
            with patch.object(regalloc.levels, "read_boot_functions", return_value=[]), \
                 patch.object(regalloc.levels, "read_catalogue", return_value=[]):
                with self.assertRaisesRegex(ValueError, "neither catalogue"):
                    regalloc.diagnose(name)

    def test_unsupported_dump_mode_is_explicit_even_if_compiler_returns_zero(self):
        with tempfile.TemporaryDirectory() as temp:
            result = subprocess_result = type("Result", (), {
                "returncode": 0, "stdout": "", "stderr": "warning: unrecognised gcc debugging option: G"})()
            with patch.object(regalloc.subprocess, "run", return_value=subprocess_result):
                with self.assertRaisesRegex(RuntimeError, "does not support.*dump mode"):
                    regalloc.run_compile(Path("/src/f.c"), Path(temp), "-O2", ("-dG",), "diagnostic")
            self.assertIn("unrecognised", (Path(temp) / "diagnostic.log").read_text())

    def test_cli_reports_unavailable_without_a_traceback(self):
        stream = io.StringIO()
        with patch.object(regalloc.levels, "select"), \
             patch.object(regalloc, "diagnose", side_effect=RuntimeError("unsupported dump mode")), \
             patch("sys.stderr", stream):
            self.assertEqual(regalloc.main(["func_00000001"]), 2)
        self.assertIn("allocator diagnostic unavailable: unsupported dump mode", stream.getvalue())

    def test_scratch_objects_compared_and_production_cache_never_called(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / "source.c"
            source.write_text("int func_00000001(void) { return 1; }\n")
            header = root / "include/common.h"
            header.parent.mkdir()
            header.write_text("/* unchanged diagnostic fixture header */\n")
            tools = root / "compiler"
            tools.mkdir()
            for name in ("cpp", "cc1", "as"):
                (tools / name).write_text("compiler identity fixture\n")
            original = source.read_bytes()
            original_header = header.read_bytes()

            def compile_(src, work, flags, dumps, tag):
                (work / "x.o").write_bytes(b"same object")
                if dumps:
                    (work / "x.i.lreg").write_text(";; Function func_00000001\nRegister 85 used 4 times across 8 insns; crosses 1 call.\n")
                    (work / "x.i.greg").write_text(";; Function func_00000001\n;; 1 regs to allocate: 85\n;; Register dispositions:\n85 in 16\n;; Hard regs used: 16\n")
                return ""

            with patch.object(regalloc.build, "ROOT", root), patch.object(regalloc.build, "BUILD", root / "build"), \
                 patch.object(regalloc.build, "COMPILER", tools), \
                 patch.object(regalloc.build, "flags_for", return_value="-O2 -G0"), \
                 patch.object(regalloc.build, "compile_c") as production, \
                 patch.object(regalloc.check, "source_of", return_value=source), \
                 patch.object(regalloc.levels, "read_boot_functions", return_value=[("func_00000001", 0, 4)]), \
                 patch.object(regalloc.levels, "read_catalogue", return_value=[]), \
                 patch.object(regalloc, "run_compile", side_effect=compile_):
                key_before = regalloc.build.compile_key(source, "-O2 -G0")
                result = regalloc.diagnose("func_00000001")
                self.assertEqual(regalloc.build.compile_key(source, "-O2 -G0"), key_before)
            self.assertTrue(result["private_objects_equal"])
            self.assertEqual(source.read_bytes(), original)
            self.assertEqual(header.read_bytes(), original_header)
            self.assertFalse((root / "build/obj").exists())
            production.assert_not_called()
            self.assertEqual(json.loads((Path(result["work"]) / "diagnostic.json").read_text())["source"], "source.c")

    def test_changed_diagnostic_object_is_refused(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / "source.c"
            source.write_text("int func_00000001(void) { return 1; }\n")

            def compile_(src, work, flags, dumps, tag):
                (work / "x.o").write_bytes(tag.encode())
                return ""

            with patch.object(regalloc.build, "ROOT", root), patch.object(regalloc.build, "BUILD", root / "build"), \
                 patch.object(regalloc.build, "flags_for", return_value="-O2"), \
                 patch.object(regalloc.check, "source_of", return_value=source), \
                 patch.object(regalloc.levels, "read_boot_functions", return_value=[("func_00000001", 0, 4)]), \
                 patch.object(regalloc.levels, "read_catalogue", return_value=[]), \
                 patch.object(regalloc, "run_compile", side_effect=compile_):
                with self.assertRaisesRegex(RuntimeError, "changed object bytes"):
                    regalloc.diagnose("func_00000001")


if __name__ == "__main__":
    unittest.main()
