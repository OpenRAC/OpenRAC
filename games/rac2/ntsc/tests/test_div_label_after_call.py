"""The division label floor after a noreorder group (pad_div_erratum_nops.py, rule D).

A division two slots after a code label gets nops up to that distance. GCC writes each call
with its delay slot in `.set noreorder`, so with -g the line label of the next statement
follows an unreordered instruction; gas's own reset for that case must not cancel the floor.
Ordinary CI runs the transformer test. The output test needs the newly built `as`: it skips
without it, and RAC2_COMPILER_ROOT naming a directory without it is an error.
"""
import importlib.util
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import elf  # noqa: E402

spec = importlib.util.spec_from_file_location("pad_div_erratum_nops",
    ROOT / "tools/compiler/transformers/pad_div_erratum_nops.py")
transformer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(transformer)

NOP = 0
DIV_S = 0x46000003          # div.s with its register fields cleared
JAL = 3


def words(data):
    return [w for (w,) in struct.iter_unpack("<I", data)]


class TransformerTests(unittest.TestCase):
    def test_the_label_floor_follows_the_noreorder_reset(self):
        reset = "      if (prev_insn_unreordered)\n\tnops = 0;\n"
        self.assertIn(reset, transformer.PAD_ANCHOR)
        self.assertIn("rac2_div_code_distance () < 2", transformer.LABEL_PAD)
        self.assertNotIn("rac2_div_code_distance", transformer.PAD)
        source = transformer.PAD_ANCHOR
        patched = source.replace(transformer.PAD_ANCHOR,
                                 transformer.PAD + transformer.PAD_ANCHOR + transformer.LABEL_PAD, 1)
        self.assertLess(patched.index(reset), patched.index("rac2_div_code_distance () < 2"))


class AssemblerOutputTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = Path(os.environ.get("RAC2_COMPILER_ROOT", ROOT / "build/compiler/tools")).resolve()
        assembler = compiler / "as"
        if not assembler.is_file():
            if "RAC2_COMPILER_ROOT" in os.environ:
                raise RuntimeError("RAC2_COMPILER_ROOT must contain the newly built as")
            raise unittest.SkipTest("EE assembler absent; tools/compiler/build.sh runs these checks after building it")
        cls.bodies = {}
        with tempfile.TemporaryDirectory() as d:
            output = Path(d) / "authored.o"
            try:
                result = subprocess.run([str(assembler), "-o", str(output),
                                         str(ROOT / "tests/fixtures/div_label_after_call.s")],
                                        capture_output=True, text=True)
            except OSError:
                if "RAC2_COMPILER_ROOT" in os.environ:
                    raise
                raise unittest.SkipTest("EE assembler not runnable on this host")
            if result.returncode:
                raise RuntimeError(f"authored assembly failed: {result.stderr}")
            obj = elf.read_object(output)
            for symbol in obj["symbols"]:
                if symbol["name"].endswith("_call") or symbol["name"] == "label_in_noreorder":
                    data = obj["sections"][symbol["section"]]["data"]
                    cls.bodies[symbol["name"]] = words(data[symbol["value"]:symbol["value"] + symbol["size"]])

    def shape(self, name):
        out = []
        for w in self.bodies[name]:
            if w == NOP:
                out.append("nop")
            elif w >> 26 == JAL:
                out.append("jal")
            elif w & 0xFFE0003F == DIV_S:
                out.append("div.s")
            else:
                out.append("other")
        return out

    def test_a_label_after_a_call_gets_two_nops(self):
        self.assertEqual(self.shape("label_after_call"), ["jal", "other", "nop", "nop", "div.s"])

    def test_no_label_means_no_padding(self):
        self.assertEqual(self.shape("no_label_after_call"), ["jal", "other", "div.s"])

    def test_a_division_inside_noreorder_is_left_alone(self):
        self.assertEqual(self.shape("label_in_noreorder"), ["jal", "other", "div.s"])


if __name__ == "__main__":
    unittest.main()
