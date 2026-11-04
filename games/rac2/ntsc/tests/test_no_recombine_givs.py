"""strength_reduce does not recombine givs: transformer drift and EE compiler output."""
import importlib.util
import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location("no_recombine_givs",
    ROOT / "tools/compiler/transformers/no_recombine_givs.py")
transformer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(transformer)

FUNCTIONS = ("scan", "count")


def function_text(asm: str, name: str) -> str:
    return re.search(r"(?ms)^" + name + r":.*?\.end\s+" + name, asm).group()


class TransformerTests(unittest.TestCase):
    def test_the_call_is_removed_once(self):
        with tempfile.TemporaryDirectory() as d:
            loop = Path(d) / "loop.c"
            loop.write_text("before\n" + transformer.CALL_OLD + "after\n")
            transformer.patch(loop)
            text = loop.read_text()
            self.assertNotIn("recombine_givs (bl", text)
            self.assertNotIn("flag_rac2", text)
            with self.assertRaisesRegex(ValueError, "found 0"):
                transformer.patch(loop)

    def test_source_drift_is_rejected_without_writing(self):
        with tempfile.TemporaryDirectory() as d:
            loop = Path(d) / "loop.c"
            loop.write_text("unexpected source\n")
            with self.assertRaisesRegex(ValueError, "found 0"):
                transformer.patch(loop)
            self.assertEqual(loop.read_text(), "unexpected source\n")


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
            source.write_bytes((ROOT / "tests/fixtures/no_recombine_givs.c").read_bytes())
            with (root / "input.i").open("w") as out:
                try:
                    subprocess.run([str(compiler / "cpp"), str(source)], stdout=out, check=True)
                except OSError:
                    if "RAC2_COMPILER_ROOT" in os.environ:
                        raise
                    raise unittest.SkipTest("EE compiler not runnable on this host; tools/compiler/build.sh "
                                            "runs these checks in its build environment")
            asm = root / "input.s"
            subprocess.run([str(compiler / "cc1"), "-quiet", "-O2", "-G8", "-ffunction-sections",
                            str(root / "input.i"), "-o", str(asm)], check=True)
            subprocess.run([str(compiler / "as"), "-o", str(root / "input.o"), str(asm)], check=True)
            text = asm.read_text()
            cls.outputs = {name: function_text(text, name) for name in FUNCTIONS}

    def test_the_digit_read_stays_on_the_entry_pointer(self):
        # Recombination moved it onto name + 5 as `lbu $2,-1($16)'.
        text = self.outputs["scan"]
        size = re.search(r"lw\t\$\d+,16\((\$\d+)\)", text).group(1)
        self.assertRegex(text, r"lbu\t\$\d+,36\(" + re.escape(size) + r"\)")
        self.assertNotIn(",-1(", text)

    def test_a_single_pointer_loop_is_unchanged(self):
        self.assertRegex(self.outputs["count"], r"lbu\t\$\d+,20\(\$\d+\)")


if __name__ == "__main__":
    unittest.main()
