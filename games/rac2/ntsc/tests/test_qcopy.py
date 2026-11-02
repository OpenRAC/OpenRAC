"""The grouped-copy option must be isolated, safe for its scratch, and reversible."""
import importlib.util
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import elf
spec = importlib.util.spec_from_file_location("qcopy_pattern", ROOT / "tools/compiler/transformers/qcopy_pattern.py")
transformer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(transformer)


class SourcePatchTests(unittest.TestCase):
    def test_drift_does_not_partially_modify_sources(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            (root / "config/mips").mkdir(parents=True)
            files = {"flags.h": "extern int flag_rac2_zero_ti_barrier;",
                     "toplev.c": 'int flag_rac2_zero_ti_barrier = 0;\n'
                                 '  {"rac2-zero-ti-barrier", &flag_rac2_zero_ti_barrier, 1,',
                     "config/mips/mips.md": "unexpected machine description"}
            for name, text in files.items():
                (root / name).write_text(text)
            with self.assertRaises(ValueError):
                transformer.patch(root)
            self.assertEqual({name: (root / name).read_text() for name in files}, files)


class CompilerCopyTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = Path(os.environ.get("RAC2_COMPILER_ROOT", ROOT / "build/compiler/tools"))
        if not all((compiler / name).is_file() for name in ("cpp", "cc1", "as")):
            if "RAC2_COMPILER_ROOT" in os.environ:
                raise RuntimeError("RAC2_COMPILER_ROOT must contain the new EE compiler and assembler")
            raise unittest.SkipTest("EE compiler absent; the compiler recipe requires these controls")
        cls.outputs = {}
        cls.words = {}
        with tempfile.TemporaryDirectory() as d:
            work = Path(d)
            source = work / "control.c"
            source.write_bytes((ROOT / "tests/fixtures/qcopy.c").read_bytes())
            pre = work / "control.i"
            with pre.open("wb") as out:
                subprocess.run([str(compiler / "cpp"), str(source)], stdout=out, check=True)
            for profile, flags in (("default", []), ("enabled", ["-frac2-qcopy"]),
                                   ("disabled", ["-frac2-qcopy", "-fno-rac2-qcopy"])):
                asm = work / (profile + ".s")
                subprocess.run([str(compiler / "cc1"), "-quiet", "-O2", "-G0", "-ffunction-sections",
                                *flags, str(pre), "-o", str(asm)], check=True)
                obj_path = work / (profile + ".o")
                subprocess.run([str(compiler / "as"), "-o", str(obj_path), str(asm)], check=True)
                obj = elf.read_object(obj_path)
                cls.words[profile] = {section["name"]: struct.unpack("<" + "I" * (len(section["data"]) // 4), section["data"])
                    for section in obj["sections"] if section["name"].startswith(".text.")}
                text = asm.read_text()
                cls.outputs[profile] = {name: re.search(r"(?ms)^" + name + r":.*?\.end\s+" + name, text).group()
                    for name in ("pointers", "offsets", "negative", "scalar", "wide", "zero",
                                 "returned_address", "volatile_access", "conditional", "local", "fp_conditional", "fp_choice")}

    def test_disabled_restores_default(self):
        self.assertEqual(self.outputs["default"], self.outputs["disabled"])

    def test_scalar_and_zero_stores_are_unchanged(self):
        for name in ("scalar", "wide", "zero"):
            with self.subTest(name=name):
                self.assertEqual(self.outputs["default"][name], self.outputs["enabled"][name])

    def test_copy_uses_complete_pair_and_does_not_destroy_an_address(self):
        for name in ("pointers", "offsets", "negative", "returned_address", "conditional", "fp_conditional", "fp_choice"):
            with self.subTest(name=name):
                body = self.outputs["enabled"][name]
                pairs = re.findall(r"lq\s+\$2,0\((\$\d+)\)\s+sq\s+\$2,0\((\$\d+)\)", body)
                self.assertTrue(pairs)
                for src, dst in pairs:
                    self.assertNotEqual(src, "$2")
                    self.assertNotEqual(dst, "$2")

    def test_offsets_are_computed_and_never_folded_in_the_pair(self):
        for name, offset in (("offsets", 16), ("negative", -16)):
            with self.subTest(name=name):
                body = self.outputs["enabled"][name]
                self.assertRegex(body, rf"addu\s+\$\d+,\$\d+,{offset}\b")
                self.assertRegex(body, r"addu\s+\$\d+,\$\d+,32\b")
                self.assertNotRegex(body, r"[ls]q\s+\$\d+,(?:16|-16|32)\(")

    def test_volatile_copy_keeps_both_memory_accesses(self):
        body = self.outputs["enabled"]["volatile_access"]
        self.assertEqual(len(re.findall(r"\blq\s", body)), 2)
        self.assertEqual(len(re.findall(r"\bsq\s", body)), 2)

    def test_float_controls_reach_cop1_branches(self):
        for name in ("fp_conditional", "fp_choice"):
            words = self.words["enabled"][".text." + name]
            self.assertTrue(any(word >> 26 == 17 and (word >> 21) & 31 == 8
                                for word in words), name)

    def test_pair_does_not_start_in_a_branch_delay_slot(self):
        # Check assembled words: gas can insert a valid delay nop in reorder mode.
        for name, words in self.words["enabled"].items():
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
