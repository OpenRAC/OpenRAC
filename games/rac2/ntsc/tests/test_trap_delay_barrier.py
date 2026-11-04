"""reorg's delay-slot search stops at a division guard: transformer drift and EE compiler output."""
import importlib.util
import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location("trap_delay_barrier",
    ROOT / "tools/compiler/transformers/trap_delay_barrier.py")
transformer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(transformer)

FUNCTIONS = ("guarded_call", "plain_call")


def function_text(asm: str, name: str) -> str:
    """The function's assembly, its local labels renumbered in order of appearance."""
    body = re.search(r"(?ms)^" + name + r":.*?\.end\s+" + name, asm).group()
    labels = {}
    return re.sub(r"\$L\d+", lambda m: labels.setdefault(m.group(), f"$L{len(labels)}"), body)


def call_slot(text: str) -> str:
    """The instruction GCC put in the `jal` slot, or "" when it left the slot to the assembler."""
    lines = [l.strip() for l in text.splitlines()]
    i = next(k for k, l in enumerate(lines) if l.startswith("jal\t"))
    if lines[i - 1] != ".set\tnomacro":
        return ""
    return lines[i + 1].split("#")[0].strip()


def guard_index(text: str) -> int:
    return text.index("break\t7")


class TransformerTests(unittest.TestCase):
    def test_the_insertion_point_is_used_once(self):
        with tempfile.TemporaryDirectory() as d:
            reorg = Path(d) / "reorg.c"
            reorg.write_text("before\n" + transformer.SCAN_OLD + "\nafter\n")
            transformer.patch(reorg)
            text = reorg.read_text()
            self.assertIn("\t      if (GET_CODE (trial) == INSN\n"
                          "\t\t  && GET_CODE (PATTERN (trial)) == TRAP_IF)\n\t\tbreak;\n", text)
            self.assertNotIn("flag_rac2", text)
            with self.assertRaisesRegex(ValueError, "fill_simple_delay_slots"):
                transformer.patch(reorg)

    def test_source_drift_is_rejected_without_writing(self):
        with tempfile.TemporaryDirectory() as d:
            reorg = Path(d) / "reorg.c"
            reorg.write_text("unexpected source\n")
            with self.assertRaisesRegex(ValueError, "found 0"):
                transformer.patch(reorg)
            self.assertEqual(reorg.read_text(), "unexpected source\n")


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
            source.write_bytes((ROOT / "tests/fixtures/trap_delay_barrier.c").read_bytes())
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

    def test_the_argument_stays_above_the_guard(self):
        # This tree's search moved `li $4,2' past the guard into the `jal' slot.
        text = self.outputs["guarded_call"]
        self.assertEqual(call_slot(text), "")
        self.assertLess(text.index("li\t$4,2"), guard_index(text))

    def test_without_a_guard_the_slot_is_filled(self):
        self.assertNotIn("break\t7", self.outputs["plain_call"])
        self.assertNotEqual(call_slot(self.outputs["plain_call"]), "")


if __name__ == "__main__":
    unittest.main()
