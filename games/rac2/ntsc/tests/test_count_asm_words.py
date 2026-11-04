"""Loop padding counts an asm statement and the grouped qcopy by their words."""
import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent.parent
FUNCTIONS = ("asm_copy_rows", "pattern_copy_rows", "scalar_rows")


def loop_words(asm: str, name: str) -> tuple[int, int]:
    """(words from the loop label through the branch's delay slot, NOPs among them)."""
    body = re.search(r"(?ms)^" + name + r":.*?\.end\s+" + name, asm).group()
    lines = [l.split("#")[0].strip() for l in body.splitlines()]
    start = next(k for k, l in enumerate(lines) if re.match(r"\$L\d+:", l))
    words = nops = 0
    for k in range(start + 1, len(lines)):
        line = lines[k]
        if not line or line.startswith(".") or line.endswith(":"):
            continue
        words += 1
        nops += line == "nop"
        if re.match(r"b\w*\s", line):
            return words + 1, nops   # the delay slot follows
    raise AssertionError("no loop branch in " + name)


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
            source.write_bytes((ROOT / "tests/fixtures/count_asm_words.c").read_bytes())
            with (root / "input.i").open("w") as out:
                try:
                    subprocess.run([str(compiler / "cpp"), str(source)], stdout=out, check=True)
                except OSError:
                    if "RAC2_COMPILER_ROOT" in os.environ:
                        raise
                    raise unittest.SkipTest("EE compiler not runnable on this host; tools/compiler/build.sh "
                                            "runs these checks in its build environment")
            asm = root / "input.s"
            subprocess.run([str(compiler / "cc1"), "-quiet", "-O2", "-G0", "-ffunction-sections",
                            "-frac2-qcopy", str(root / "input.i"), "-o", str(asm)], check=True)
            subprocess.run([str(compiler / "as"), "-o", str(root / "input.o"), str(asm)], check=True)
            text = asm.read_text()
            cls.loops = {name: loop_words(text, name) for name in FUNCTIONS}

    def test_the_asm_copy_counts_two_words(self):
        # Counted as one insn, the seven-word retail loop got an eighth word.
        words, nops = self.loops["asm_copy_rows"]
        self.assertEqual((words, nops), (7, 0))

    def test_the_grouped_copy_counts_two_words(self):
        words, nops = self.loops["pattern_copy_rows"]
        self.assertEqual(words, 7)
        self.assertLessEqual(nops, 1)

    def test_a_short_scalar_loop_is_still_padded(self):
        words, nops = self.loops["scalar_rows"]
        self.assertGreaterEqual(words, 7)


if __name__ == "__main__":
    unittest.main()
