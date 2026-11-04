"""The opt-in --r5900-swap-branch-slots assembler profile: transformer drift and EE assembler output."""
import importlib.util
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import elf

spec = importlib.util.spec_from_file_location("swap_branch_slots",
    ROOT / "tools/compiler/transformers/swap_branch_slots.py")
transformer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(transformer)

SWAPPED = ("store_call", "load_call", "trunc_return")
CONTROLS = ("dependent_return", "labelled_return")
NOP = b"\0\0\0\0"


def words(body: bytes) -> list[int]:
    return [int.from_bytes(body[i:i + 4], "little") for i in range(0, len(body), 4)]


class TransformerTests(unittest.TestCase):
    def source(self) -> str:
        return "".join(old for old, _ in transformer.EDITS)

    def test_every_insertion_point_is_used_once(self):
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / "tc-mips.c"
            path.write_text("before\n" + self.source() + "after\n")
            transformer.patch(path)
            text = path.read_text()
            self.assertIn("? (prev_pinfo & INSN_SYNC) != 0\n", text)
            self.assertIn(": (prev_pinfo && INSN_SYNC)))\n", text)
            self.assertIn("static int r5900_swap_branch_slots = 0;\n", text)
            self.assertIn('{"r5900-swap-branch-slots", no_argument', text)
            with self.assertRaisesRegex(ValueError, "found 0"):
                transformer.patch(path)

    def test_source_drift_is_rejected_without_writing(self):
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / "tc-mips.c"
            path.write_text("unexpected source\n")
            with self.assertRaisesRegex(ValueError, "found 0"):
                transformer.patch(path)
            self.assertEqual(path.read_text(), "unexpected source\n")


class AssemblerOutputTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = Path(os.environ.get("RAC2_COMPILER_ROOT", ROOT / "build/compiler/tools")).resolve()
        assembler = compiler / "as"
        if not assembler.is_file():
            if "RAC2_COMPILER_ROOT" in os.environ:
                raise RuntimeError("RAC2_COMPILER_ROOT must contain the newly built as")
            raise unittest.SkipTest("EE assembler absent; tools/compiler/build.sh runs these checks after building it")
        cls.profiles = {}
        with tempfile.TemporaryDirectory() as d:
            output = Path(d) / "authored.o"
            for profile, options in (("default", []), ("swap", ["--r5900-swap-branch-slots"])):
                try:
                    result = subprocess.run([str(assembler), *options, "-o", str(output),
                                             str(ROOT / "tests/fixtures/swap_branch_slots.s")],
                                            capture_output=True, text=True)
                except OSError:
                    if "RAC2_COMPILER_ROOT" in os.environ:
                        raise
                    raise unittest.SkipTest("EE assembler not runnable on this host")
                if result.returncode:
                    raise RuntimeError(f"authored assembly failed ({profile}): {result.stderr}")
                obj = elf.read_object(output)
                bodies = {}
                for symbol in obj["symbols"]:
                    if symbol["name"] in SWAPPED + CONTROLS:
                        data = obj["sections"][symbol["section"]]["data"]
                        bodies[symbol["name"]] = data[symbol["value"]:symbol["value"] + symbol["size"]]
                cls.profiles[profile] = bodies

    def test_the_default_assembler_never_swaps(self):
        # 2.9-ee-991111's guard reads `prev_pinfo && INSN_SYNC`: every slot gets a nop.
        for name in SWAPPED + CONTROLS:
            with self.subTest(function=name):
                self.assertTrue(self.profiles["default"][name].endswith(NOP))

    def test_the_profile_moves_the_previous_instruction_into_the_slot(self):
        for name in SWAPPED:
            with self.subTest(function=name):
                default = words(self.profiles["default"][name])
                swapped = words(self.profiles["swap"][name])
                self.assertEqual(default[-1], 0)
                self.assertEqual(swapped, default[:-3] + [default[-2], default[-3]])

    def test_the_store_and_the_sign_extension_words(self):
        swap = self.profiles["swap"]
        self.assertEqual(words(swap["store_call"])[1], 0xACA40000)       # sw $4,0($5)
        self.assertEqual(words(swap["load_call"])[1], 0x8CA40000)        # lw $4,0($5)
        self.assertEqual(words(swap["trunc_return"]), [0x0004103C,      # dsll32 $2,$4,0
                                                       0x03E00008,      # jr $31
                                                       0x0002103F])     # dsra32 $2,$2,0

    def test_dependences_and_labels_still_refuse_the_swap(self):
        for name in CONTROLS:
            with self.subTest(function=name):
                self.assertEqual(self.profiles["swap"][name], self.profiles["default"][name])


if __name__ == "__main__":
    unittest.main()
