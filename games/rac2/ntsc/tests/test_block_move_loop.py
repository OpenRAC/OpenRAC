"""Block moves count their MD length in the R5900 short-loop padding: transformer drift and EE output."""
import importlib.util
import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location("count_block_move_length",
    ROOT / "tools/compiler/transformers/count_block_move_length.py")
transformer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(transformer)

FUNCTIONS = ("copy_page", "copy_aligned_page", "clear_words")
PAD_LOOPS = "void\nmips_r5900_pad_loops (first)\n{\n%s\n}\n/* ASTRA P25: next */\n"


def function_text(asm: str, name: str) -> str:
    return re.search(r"(?ms)^" + name + r":.*?\.end\s+" + name, asm).group()


def loop_nops(text: str) -> list:
    """For each backward branch, the nops placed right before it (the padding)."""
    lines = [l.strip() for l in text.split("\n")]
    labels = {l[:-1]: i for i, l in enumerate(lines) if re.fullmatch(r"\$L\d+:", l)}
    counts = []
    for i, l in enumerate(lines):
        m = re.match(r"b\w*\s.*,(\$L\d+)$", l)
        if m and labels.get(m.group(1), i + 1) < i:
            n, j = 0, i - 1
            while j >= 0 and lines[j] in ("nop", ".set\tnoreorder", ".set\tnomacro", ".set\treorder", ".set\tmacro", ""):
                n += lines[j] == "nop"
                j -= 1
            counts.append(n)
    return counts


class TransformerTests(unittest.TestCase):
    def test_the_counter_is_replaced_once(self):
        with tempfile.TemporaryDirectory() as d:
            mips = Path(d) / "mips.c"
            mips.write_text(PAD_LOOPS % transformer.OLD)
            transformer.patch(mips)
            text = mips.read_text()
            self.assertIn("BLKmode", text)
            self.assertNotIn(transformer.OLD, text)
            with self.assertRaisesRegex(ValueError, "found 0"):
                transformer.patch(mips)

    def test_source_drift_is_rejected_without_writing(self):
        with tempfile.TemporaryDirectory() as d:
            mips = Path(d) / "mips.c"
            mips.write_text(PAD_LOOPS % "\t  n++;")
            with self.assertRaisesRegex(ValueError, "found 0"):
                transformer.patch(mips)
            self.assertEqual(mips.read_text(), PAD_LOOPS % "\t  n++;")


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
            source.write_bytes((ROOT / "tests/fixtures/block_move_loop.c").read_bytes())
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

    def test_unaligned_block_move_loops_are_not_padded(self):
        # Both loops (ldl/ldr and ld/sd) are 19 and 11 words long in retail, e.g.
        # func_L08_003C6AB0, func_002F7738, func_L30_0040B9D8: no NOPs before the branch.
        self.assertEqual(loop_nops(self.outputs["copy_page"]), [0, 0])

    def test_aligned_block_move_loop_is_not_padded(self):
        self.assertEqual(loop_nops(self.outputs["copy_aligned_page"]), [0])

    def test_a_short_loop_is_still_padded(self):
        self.assertEqual(loop_nops(self.outputs["clear_words"]), [3])


if __name__ == "__main__":
    unittest.main()
