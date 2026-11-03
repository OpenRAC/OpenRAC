"""ti_one_gpr.py: the transformer's anchor, and a TImode reload taking one general register."""
import importlib.util
import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent.parent

spec = importlib.util.spec_from_file_location("ti_one_gpr", ROOT / "tools/compiler/transformers/ti_one_gpr.py")
transformer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(transformer)

NAMES = ("rotate", "stack", "single")


class TransformerTests(unittest.TestCase):
    HEADER = "#define CLASS_UNITS(mode, size)\n\n" + transformer.OLD + "\n/* next */\n"

    def test_patch_counts_timode_once_in_general_classes_and_applies_once(self):
        patched = transformer.patch(self.HEADER)
        macro = patched.split("#define CLASS_MAX_NREGS", 1)[1].split("\n\n", 1)[0]
        # FP_REGS keeps its count; TImode in GR_REGS or a subset before it takes one register;
        # every other class and mode keeps the word count.
        self.assertRegex(macro, r"\(CLASS\) == FP_REGS\s*\\\n\s*\? \(TARGET_FLOAT64")
        self.assertRegex(macro, r": \(TARGET_MIPS5900 && \(MODE\) == TImode\s*\\\n"
                                r"\s*&& \(int\) \(CLASS\) <= \(int\) GR_REGS\)\s*\\\n\s*\? 1\s*\\\n"
                                r"\s*: CLASS_UNITS \(MODE, UNITS_PER_WORD\)\)$")
        self.assertTrue(patched.endswith("\n/* next */\n"))
        with self.assertRaisesRegex(ValueError, "already patched"):
            transformer.patch(patched)

    def test_source_drift_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "expected one definition, found 0"):
            transformer.patch(self.HEADER.replace("UNITS_PER_WORD", "UNITS_PER_FPREG"))
        with self.assertRaisesRegex(ValueError, "found 2"):
            transformer.patch(self.HEADER + transformer.OLD)


def instructions(body: str) -> list[str]:
    lines = (line.split("#")[0].strip() for line in body.splitlines())
    return [line for line in lines if line and not line.startswith(".") and not line.endswith(":")]


class CompilerOutputTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = Path(os.environ.get("RAC2_COMPILER_ROOT", ROOT / "build/compiler/tools"))
        if not all((compiler / name).is_file() for name in ("cpp", "cc1", "as")):
            if "RAC2_COMPILER_ROOT" in os.environ:
                raise RuntimeError("RAC2_COMPILER_ROOT must contain the newly built cpp, cc1 and as")
            raise unittest.SkipTest("EE compiler absent; tools/compiler/build.sh runs these checks after building it")
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            source = root / "input.c"
            source.write_bytes((ROOT / "tests/fixtures/ti_one_gpr.c").read_bytes())
            with (root / "input.i").open("w") as out:
                subprocess.run([str(compiler / "cpp"), str(source)], stdout=out, check=True)
            asm = root / "input.s"
            subprocess.run([str(compiler / "cc1"), "-quiet", "-O2", "-G0", "-ffunction-sections",
                            str(root / "input.i"), "-o", str(asm)], check=True)
            subprocess.run([str(compiler / "as"), "-o", str(root / "input.o"), str(asm)], check=True)
            text = asm.read_text()
        cls.code = {name: instructions(re.search(r"(?ms)^" + name + r":.*?\.end\s+" + name, text).group())
                    for name in NAMES}

    def zeros(self, name):
        return [re.fullmatch(r"por\s+(\$\d+),\$0,\$0", line).group(1)
                for line in self.code[name] if line.startswith("por")]

    def stores(self, name):
        return [re.match(r"sq\s+(\$\d+),", line).group(1) for line in self.code[name] if line.startswith("sq")]

    def test_zero_reloads_rotate_through_single_registers(self):
        # With two-register groups reload skips $3 and alternates $5 ($a1) and $2 ($v0).
        self.assertEqual(self.zeros("rotate"), ["$3", "$2", "$3", "$2", "$3"])
        self.assertEqual(self.stores("rotate"), self.zeros("rotate"))

    def test_a_shared_stack_zero_takes_v1(self):
        # With two-register groups the zero takes $5 ($a1).
        self.assertEqual(self.zeros("stack"), ["$3"])
        self.assertEqual(self.stores("stack"), ["$3", "$3"])
        self.assertIn("ori\t$5,$5,0x3080", self.code["stack"])

    def test_a_lone_zero_takes_the_first_spill_register(self):
        self.assertEqual(self.zeros("single"), ["$2"])
        self.assertEqual(self.stores("single"), ["$2"])


if __name__ == "__main__":
    unittest.main()
