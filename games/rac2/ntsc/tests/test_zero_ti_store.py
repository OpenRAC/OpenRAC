"""Clear-selector boundaries and optional EE compiler-output regressions."""
import importlib.util
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location("zero_ti_store_barrier",
    ROOT / "tools/compiler/transformers/zero_ti_store_barrier.py")
transformer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(transformer)


class SelectorTests(unittest.TestCase):
    def test_selector_boundaries_execute_the_actual_compiler_helper(self):
        compiler = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
        if not compiler:
            self.skipTest("requires a host C compiler; CI and the build image provide one")
        # Compile the injected helper itself against a minimal RTL object model.
        # Expected cases below are independent of its implementation text.
        helpers = transformer.new.split("/* Analyze an INSN")[0]
        source = r'''
#include <assert.h>
enum { SET, MEM, REG, CONST_INT, INSN, NOTE, CODE_LABEL, JUMP_INSN, CALL_INSN, BARRIER };
enum { SImode, TImode };
typedef struct Node *rtx;
struct Node { int code, mode, regno; rtx dest, src, prev, next, pattern; };
#define GET_CODE(x) ((x)->code)
#define GET_MODE(x) ((x)->mode)
#define SET_DEST(x) ((x)->dest)
#define SET_SRC(x) ((x)->src)
#define REGNO(x) ((x)->regno)
#define PREV_INSN(x) ((x)->prev)
#define NEXT_INSN(x) ((x)->next)
#define PATTERN(x) ((x)->pattern)
struct Node zero = {CONST_INT};
rtx const0_rtx = &zero;
int reload_completed;
''' + helpers + r'''
int main(void) {
    struct Node memory = {MEM, TImode}, reg = {REG, TImode, 0};
    struct Node set = {SET, 0, 0, &memory, &zero};
    struct Node insn = {INSN, 0, 0, 0, 0, 0, 0, &set};
    struct Node other = {INSN, 0, 0, 0, 0, 0, 0, &set};
    struct Node boundary = {NOTE};
    int kinds[] = {CODE_LABEL, JUMP_INSN, CALL_INSN, BARRIER};
    unsigned i;
    assert(rac2_zero_ti_barrier_p(&set, &insn));
    insn.prev = &other;
    assert(!rac2_zero_ti_barrier_p(&set, &insn));
    insn.prev = 0; insn.next = &other;
    assert(!rac2_zero_ti_barrier_p(&set, &insn));
    for (i = 0; i < sizeof(kinds)/sizeof(kinds[0]); ++i) {
        boundary.code = kinds[i];
        boundary.prev = &other; boundary.next = 0;
        insn.prev = &boundary; insn.next = 0;
        assert(rac2_zero_ti_barrier_p(&set, &insn));
        boundary.prev = 0; boundary.next = &other;
        insn.prev = 0; insn.next = &boundary;
        assert(rac2_zero_ti_barrier_p(&set, &insn));
    }
    boundary.code = NOTE;
    assert(!rac2_zero_ti_barrier_p(&set, &insn));
    insn.next = 0;
    memory.mode = SImode;
    assert(!rac2_zero_ti_barrier_p(&set, &insn));
    memory.mode = TImode; memory.code = REG;
    assert(!rac2_zero_ti_barrier_p(&set, &insn));
    memory.code = MEM; reload_completed = 1;
    assert(!rac2_zero_ti_barrier_p(&set, &insn));
    set.src = &reg;
    assert(rac2_zero_ti_barrier_p(&set, &insn));
    reg.regno = 2;
    assert(!rac2_zero_ti_barrier_p(&set, &insn));
    return 0;
}
'''
        with tempfile.TemporaryDirectory() as d:
            src, exe = Path(d) / "selector.c", Path(d) / "selector"
            src.write_text(source)
            subprocess.run([compiler, "-std=c89", str(src), "-o", str(exe)], check=True,
                           capture_output=True, text=True)
            subprocess.run([str(exe)], check=True, capture_output=True, text=True)

    def test_source_drift_is_rejected_before_any_file_is_written(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            files = {"haifa-sched.c": transformer.old,
                     "toplev.c": "int flag_schedule_insns_after_reload = 0;\n"
                                 '  {"schedule-insns", &flag_schedule_insns, 1,\n',
                     "flags.h": "unexpected declaration"}
            for name, contents in files.items():
                (root / name).write_text(contents)
            with self.assertRaisesRegex(ValueError, "option declaration"):
                transformer.patch(root / "haifa-sched.c")
            self.assertEqual({name:(root/name).read_text() for name in files}, files)


class CompilerOutputTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = Path(os.environ.get("RAC2_COMPILER_ROOT", ROOT / "build/compiler/tools"))
        if not all((compiler / name).is_file() for name in ("cpp", "cc1", "as")):
            if "RAC2_COMPILER_ROOT" in os.environ:
                raise RuntimeError("RAC2_COMPILER_ROOT must contain the newly built cpp, cc1 and as")
            raise unittest.SkipTest("EE compiler absent; tools/compiler/build.sh runs these checks after building it")
        cls.outputs = {}
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            source = root / "input.c"
            source.write_bytes((ROOT / "tests/fixtures/zero_ti_store.c").read_bytes())
            with (root / "input.i").open("w") as out:
                subprocess.run([str(compiler / "cpp"), str(source)], stdout=out, check=True)
            for label, options in (("default", []), ("enabled", ["-frac2-zero-ti-barrier"]),
                                   ("disabled", ["-frac2-zero-ti-barrier", "-fno-rac2-zero-ti-barrier"])):
                asm = root / (label + ".s")
                subprocess.run([str(compiler / "cc1"), "-quiet", "-O2", "-G0", "-ffunction-sections",
                                *options, str(root / "input.i"), "-o", str(asm)], check=True)
                subprocess.run([str(compiler / "as"), "-o", str(root / (label+".o")), str(asm)], check=True)
                text = asm.read_text()
                cls.outputs[label] = {name:re.search(r"(?ms)^"+name+r":.*?\.end\s+"+name, text).group()
                                      for name in ("isolated", "pair", "offset", "negative_offset", "calls", "branch")}

    def test_disabled_option_restores_default_for_every_control(self):
        self.assertEqual(self.outputs["default"], self.outputs["disabled"])

    def test_isolated_and_call_separated_clears_exercise_the_option(self):
        for name in ("isolated", "calls"):
            with self.subTest(control=name):
                self.assertNotEqual(self.outputs["default"][name], self.outputs["enabled"][name])
                self.assertRegex(self.outputs["enabled"][name], r"sq\s+\$0,0\(")

    def test_shared_and_branch_controls_retain_ordinary_scheduling(self):
        for name in ("pair", "branch"):
            with self.subTest(control=name):
                self.assertEqual(self.outputs["default"][name], self.outputs["enabled"][name])
        self.assertIn("por", self.outputs["enabled"]["pair"])

    def test_nonzero_displacement_keeps_a_materialized_zero(self):
        for profile in self.outputs:
            for name, displacement in (("offset", 16), ("negative_offset", -16)):
                with self.subTest(profile=profile, displacement=displacement):
                    text = self.outputs[profile][name]
                    zero = re.search(r"por\s+(\$[0-9]+),\$0,\$0", text)
                    self.assertIsNotNone(zero)
                    self.assertRegex(text, r"sq\s+" + re.escape(zero.group(1))
                                     + rf",{displacement}\(")
                    self.assertNotRegex(text, rf"sq\s+\$0,{displacement}\(")

    def test_zero_displacement_still_folds_in_every_profile(self):
        for profile in self.outputs:
            with self.subTest(profile=profile):
                self.assertRegex(self.outputs[profile]["isolated"], r"sq\s+\$0,0\(")


if __name__ == "__main__":
    unittest.main()
