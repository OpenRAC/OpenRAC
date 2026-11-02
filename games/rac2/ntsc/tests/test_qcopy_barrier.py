"""The grouped-copy barrier: its selector, its controls and its reversibility."""
import importlib.util
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import elf  # noqa: E402


def load(name):
    spec = importlib.util.spec_from_file_location(name, ROOT / f"tools/compiler/transformers/{name}.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


transformer = load("qcopy_barrier")
clear_barrier = load("zero_ti_store_barrier")
qcopy = load("qcopy_pattern")

NAMES = ("constant_after", "neighbours", "restores_after", "arguments_after", "alone",
         "around_call", "around_branch", "volatile_neighbours", "volatile_copy")


class SelectorTests(unittest.TestCase):
    def test_selector_accepts_only_the_grouped_copy(self):
        compiler = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
        if not compiler:
            self.skipTest("requires a host C compiler; CI and the build image provide one")
        # Compile the injected helper itself against a minimal RTL object model.
        # Expected cases below are independent of its implementation text.
        source = r'''
#include <assert.h>
enum { SET, MEM, REG, CLOBBER, USE, PARALLEL, CONST_INT, ASM_OPERANDS };
enum { QImode, SImode, TImode };
typedef struct Node *rtx;
struct Node { int code, mode, regno, len; rtx op[3]; };
#define GET_CODE(x) ((x)->code)
#define GET_MODE(x) ((x)->mode)
#define XVECLEN(x, n) ((x)->len)
#define XVECEXP(x, n, i) ((x)->op[i])
#define XEXP(x, i) ((x)->op[i])
#define SET_DEST(x) ((x)->op[0])
#define SET_SRC(x) ((x)->op[1])
#define REGNO(x) ((x)->regno)
''' + transformer.HELPER + r'''
int main(void) {
    struct Node da = {REG, SImode, 4}, sa = {REG, SImode, 5};
    struct Node dst = {MEM, TImode, 0, 0, {&da}}, src = {MEM, TImode, 0, 0, {&sa}};
    struct Node set = {SET, 0, 0, 0, {&dst, &src}};
    struct Node v0 = {REG, TImode, 2}, clobber = {CLOBBER, 0, 0, 0, {&v0}};
    struct Node copy = {PARALLEL, 0, 0, 2, {&set, &clobber}};
    struct Node zero = {CONST_INT, 0}, asm_body = {ASM_OPERANDS, 0};
    struct Node other = {CLOBBER, 0, 0, 0, {&v0}};
    assert(rac2_qcopy_barrier_p(&copy));
    /* Not the pattern's PARALLEL: a lone SET, other lengths, swapped elements. */
    assert(!rac2_qcopy_barrier_p(&set));
    copy.len = 1; assert(!rac2_qcopy_barrier_p(&copy));
    copy.len = 3; copy.op[2] = &other; assert(!rac2_qcopy_barrier_p(&copy));
    copy.len = 2; copy.op[0] = &clobber; copy.op[1] = &set; assert(!rac2_qcopy_barrier_p(&copy));
    copy.op[0] = &set; copy.op[1] = &clobber; assert(rac2_qcopy_barrier_p(&copy));
    /* The inline-asm copy itself is not selected: its first element is the asm. */
    copy.op[0] = &asm_body; assert(!rac2_qcopy_barrier_p(&copy)); copy.op[0] = &set;
    /* Memory on both sides, in TImode. */
    dst.code = REG; assert(!rac2_qcopy_barrier_p(&copy)); dst.code = MEM;
    dst.mode = SImode; assert(!rac2_qcopy_barrier_p(&copy)); dst.mode = TImode;
    src.code = REG; assert(!rac2_qcopy_barrier_p(&copy)); src.code = MEM;
    src.mode = SImode; assert(!rac2_qcopy_barrier_p(&copy)); src.mode = TImode;
    set.op[1] = &zero; assert(!rac2_qcopy_barrier_p(&copy)); set.op[1] = &src;
    set.code = USE; assert(!rac2_qcopy_barrier_p(&copy)); set.code = SET;
    /* The scratch: a CLOBBER of (reg:TI 2), nothing else. */
    clobber.code = USE; assert(!rac2_qcopy_barrier_p(&copy)); clobber.code = CLOBBER;
    v0.regno = 3; assert(!rac2_qcopy_barrier_p(&copy)); v0.regno = 2;
    v0.mode = QImode; assert(!rac2_qcopy_barrier_p(&copy)); v0.mode = TImode;
    v0.code = MEM; assert(!rac2_qcopy_barrier_p(&copy)); v0.code = REG;
    assert(rac2_qcopy_barrier_p(&copy));
    return 0;
}
'''
        with tempfile.TemporaryDirectory() as d:
            src, exe = Path(d) / "selector.c", Path(d) / "selector"
            src.write_text(source)
            subprocess.run([compiler, "-std=c89", "-Wall", str(src), "-o", str(exe)], check=True,
                           capture_output=True, text=True)
            subprocess.run([str(exe)], check=True, capture_output=True, text=True)

    def test_selector_matches_the_pattern_it_models(self):
        # The selector's shape is the rac2_qcopy insn's: a TImode memory SET and (reg:TI 2).
        self.assertRegex(qcopy.PATTERN, r"\(set \(mem:TI .*\)\s+\(mem:TI .*\)\)\s+\(clobber \(reg:TI 2\)\)")

    def test_the_copy_reuses_the_clear_barrier_block(self):
        # One mechanism: the condition that guards -frac2-zero-ti-barrier's block.
        self.assertEqual(clear_barrier.new.count(transformer.CONDITION_OLD), 1)
        self.assertIn("INSN_COST (insn) = 1;", clear_barrier.new)
        self.assertIn("(flag_rac2_zero_ti_barrier && rac2_zero_ti_barrier_p (x, insn))",
                      transformer.CONDITION_NEW)

    def test_source_drift_is_rejected_before_any_file_is_written(self):
        sources = {"haifa-sched.c": transformer.HELPER_ANCHOR + transformer.CONDITION_OLD,
                   "toplev.c": "int flag_rac2_qcopy = 0;\n" + transformer.TABLE_ANCHOR + "\n",
                   "flags.h": "extern int flag_rac2_qcopy;\n"}
        for broken, label in (("haifa-sched.c", "barrier condition"), ("toplev.c", "option table"),
                              ("flags.h", "option declaration")):
            with self.subTest(drift=broken), tempfile.TemporaryDirectory() as d:
                root = Path(d)
                files = dict(sources)
                files[broken] = "unexpected text\n" + (transformer.HELPER_ANCHOR if broken == "haifa-sched.c"
                                                     else "int flag_rac2_qcopy = 0;\n" if broken == "toplev.c"
                                                     else "")
                for name, contents in files.items():
                    (root / name).write_text(contents)
                with self.assertRaisesRegex(ValueError, label):
                    transformer.patch(root)
                self.assertEqual({name: (root / name).read_text() for name in files}, files)

    def test_patch_applies_once(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            (root / "haifa-sched.c").write_text(transformer.HELPER_ANCHOR + transformer.CONDITION_OLD)
            (root / "toplev.c").write_text("int flag_rac2_qcopy = 0;\n" + transformer.TABLE_ANCHOR + "\n")
            (root / "flags.h").write_text("extern int flag_rac2_qcopy;\n")
            transformer.patch(root)
            sched = (root / "haifa-sched.c").read_text()
            self.assertEqual(sched.count("rac2_qcopy_barrier_p (x)"), 2)
            self.assertIn(transformer.CONDITION_NEW, sched)
            self.assertIn('{"rac2-qcopy-barrier", &flag_rac2_qcopy_barrier, 1,',
                          (root / "toplev.c").read_text())
            self.assertIn("extern int flag_rac2_qcopy_barrier;", (root / "flags.h").read_text())
            with self.assertRaisesRegex(ValueError, "already present"):
                transformer.patch(root)


def instructions(body: str) -> list[str]:
    """The instruction lines of one function's assembly, without directives or labels."""
    lines = (line.strip() for line in body.splitlines())
    return [line for line in lines if line and not line.startswith((".", "#")) and not line.endswith(":")]


class CompilerOutputTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = Path(os.environ.get("RAC2_COMPILER_ROOT", ROOT / "build/compiler/tools"))
        if not all((compiler / name).is_file() for name in ("cpp", "cc1", "as")):
            if "RAC2_COMPILER_ROOT" in os.environ:
                raise RuntimeError("RAC2_COMPILER_ROOT must contain the newly built cpp, cc1 and as")
            raise unittest.SkipTest("EE compiler absent; tools/compiler/build.sh runs these checks after building it")
        cls.outputs, cls.words = {}, {}
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            source = root / "input.c"
            source.write_bytes((ROOT / "tests/fixtures/qcopy_barrier.c").read_bytes())
            for name, defines in (("c", []), ("asm", ["-DASM_COPY"])):
                with (root / (name + ".i")).open("w") as out:
                    subprocess.run([str(compiler / "cpp"), *defines, str(source)], stdout=out, check=True)
            profiles = (("default", "c", []), ("qcopy", "c", ["-frac2-qcopy"]),
                        ("barrier", "c", ["-frac2-qcopy", "-frac2-qcopy-barrier"]),
                        ("barrier_only", "c", ["-frac2-qcopy-barrier"]),
                        ("disabled", "c", ["-frac2-qcopy", "-frac2-qcopy-barrier", "-fno-rac2-qcopy-barrier"]),
                        ("asm", "asm", []))
            for label, pre, options in profiles:
                asm, obj = root / (label + ".s"), root / (label + ".o")
                subprocess.run([str(compiler / "cc1"), "-quiet", "-O2", "-G0", "-ffunction-sections",
                                *options, str(root / (pre + ".i")), "-o", str(asm)], check=True)
                subprocess.run([str(compiler / "as"), "-o", str(obj), str(asm)], check=True)
                text = asm.read_text()
                cls.outputs[label] = {name: re.search(r"(?ms)^" + name + r":.*?\.end\s+" + name, text).group()
                                      for name in NAMES}
                cls.words[label] = {section["name"]: struct.unpack("<" + "I" * (len(section["data"]) // 4),
                                                                   section["data"])
                                    for section in elf.read_object(obj)["sections"]
                                    if section["name"].startswith(".text.")}

    def code(self, profile, name):
        return instructions(self.outputs[profile][name])

    def test_disabled_option_restores_the_copy_profile(self):
        self.assertEqual(self.outputs["qcopy"], self.outputs["disabled"])

    def test_barrier_without_the_copy_profile_is_the_default(self):
        self.assertEqual(self.outputs["default"], self.outputs["barrier_only"])

    def test_barrier_schedules_as_the_volatile_asm(self):
        for name in ("constant_after", "neighbours", "arguments_after", "alone", "around_branch",
                     "volatile_neighbours"):
            with self.subTest(control=name):
                self.assertEqual(self.code("barrier", name), self.code("asm", name))

    def test_instructions_stay_on_their_side_of_the_copy(self):
        def position(code, prefix):
            return next(i for i, line in enumerate(code) if line.startswith(prefix))
        # The float constant: hoisted into the address stall without the barrier, after it with.
        for profile, before in (("qcopy", True), ("barrier", False)):
            code = self.code(profile, "constant_after")
            with self.subTest(profile=profile, control="constant_after"):
                self.assertEqual(position(code, "li.s") < position(code, "lq "), before)
        # The epilogue's restores: between the address chain and the copy without the barrier.
        for profile, interleaved in (("qcopy", True), ("barrier", False)):
            code = self.code(profile, "restores_after")
            restores = [i for i, line in enumerate(code) if line.startswith("ld\t")]
            with self.subTest(profile=profile, control="restores_after"):
                self.assertEqual(min(restores) < position(code, "lq "), interleaved)
        for name in ("constant_after", "neighbours", "restores_after", "arguments_after"):
            with self.subTest(control=name):
                self.assertNotEqual(self.code("qcopy", name), self.code("barrier", name))

    def test_copy_without_neighbours_and_volatile_controls_are_unchanged(self):
        for name in ("alone", "around_branch", "volatile_neighbours", "volatile_copy"):
            with self.subTest(control=name):
                self.assertEqual(self.outputs["qcopy"][name], self.outputs["barrier"][name])
        body = self.outputs["barrier"]["volatile_copy"]
        self.assertEqual(len(re.findall(r"\blq\s", body)), 2)
        self.assertEqual(len(re.findall(r"\bsq\s", body)), 2)
        code = self.code("barrier", "volatile_neighbours")
        self.assertEqual([line.split()[0] for line in code if line.split()[0] in ("lw", "lq", "sq", "sw")],
                         ["lw", "lq", "sq", "sw"])

    def test_scheduling_only_a_redundant_move_is_still_deleted_after_a_call(self):
        # Not modelled: jump2 deletes `move $4,$16' before the call (find_equiv_reg sees $16 = $4
        # across the copy) and stops at a volatile asm, which therefore keeps it.
        barrier, asm = self.code("barrier", "around_call"), self.code("asm", "around_call")
        self.assertNotIn("move\t$4,$16", barrier)
        self.assertEqual(asm.count("move\t$4,$16"), 1)
        asm.remove("move\t$4,$16")
        self.assertEqual(barrier, asm)

    def test_pairs_stay_complete_and_out_of_delay_slots(self):
        for name in NAMES:
            body = self.outputs["barrier"][name]
            with self.subTest(control=name):
                self.assertEqual(len(re.findall(r"\blq\s", body)), len(re.findall(r"lq\s+\$2,0\(\$\d+\)\s+sq\s+\$2,0\(",
                                                                                 body)))
        for name, words in self.words["barrier"].items():
            for i in range(1, len(words) - 1):
                load, store = words[i:i + 2]
                if load >> 26 == 0x1E and store >> 26 == 0x1F and (load >> 16) & 31 == 2 and (store >> 16) & 31 == 2:
                    before = words[i - 1]
                    opcode = before >> 26
                    branch = (opcode in (1, 2, 3, 4, 5, 6, 7, 20, 21, 22, 23)
                              or (opcode == 0 and before & 63 in (8, 9))
                              or (opcode in (16, 17, 18) and (before >> 21) & 31 == 8))
                    self.assertFalse(branch, (name, i, hex(before)))


if __name__ == "__main__":
    unittest.main()
