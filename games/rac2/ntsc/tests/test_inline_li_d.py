"""li.d into GPRs inline by default, pooled with --r5900-pool-li-d (the SDK profile): transformer
drift, EE assembler output, and the build passing the opt-out only to SDK units."""
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

spec = importlib.util.spec_from_file_location("inline_li_d",
    ROOT / "tools/compiler/transformers/inline_li_d.py")
transformer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(transformer)

FUNCTIONS = ("tenth", "ten4", "simple")
VALUES = {"tenth": 0x3FB99999A0000000,   # (double)0.1f: no zero chunk in the high half
          "ten4": 0x3F1A36E2E0000000,    # (double)1e-4f: likewise
          "simple": 0x408B400000000000}  # 872.0, func_00284268's $t7 constant


def words(body: bytes) -> list[int]:
    return [int.from_bytes(body[i:i + 4], "little") for i in range(0, len(body), 4)]


def evaluate(code: list[int], reg: int) -> int:
    """Run an ori/dsll/dsll32/lui/daddiu chain building REG; fail on anything else."""
    value = 0
    for word in code:
        op, rs, rt, rd = word >> 26, (word >> 21) & 31, (word >> 16) & 31, (word >> 11) & 31
        sa, funct, imm = (word >> 6) & 31, word & 63, word & 0xFFFF
        if op == 0x0D and rt == reg:                                 # ori
            value = (value if rs == reg else 0) | imm
        elif op == 0x0F and rt == reg:                               # lui
            value = (imm << 16) - ((imm & 0x8000) << 17)
        elif op == 0 and funct in (0x38, 0x3C) and rd == reg and rt == reg:   # dsll, dsll32
            value = (value << (sa + (32 if funct == 0x3C else 0))) & ((1 << 64) - 1)
        else:
            raise AssertionError(f"unexpected word {word:08x}")
    return value & ((1 << 64) - 1)


class TransformerTests(unittest.TestCase):
    def source(self) -> str:
        return "".join(old for old, _ in transformer.EDITS)

    def test_every_insertion_point_is_used_once(self):
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / "tc-mips.c"
            path.write_text("before\n" + self.source() + "after\n")
            transformer.patch(path)
            text = path.read_text()
            self.assertIn("(r5900_inline_li_d && *args == 'F' && mips_opts.isa >= 3)", text)
            self.assertIn("static int r5900_inline_li_d = 1;\n", text)
            self.assertIn('{"r5900-inline-li-d", no_argument', text)
            self.assertIn('{"r5900-pool-li-d", no_argument', text)
            self.assertIn("r5900_inline_li_d = 0;", text)
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
        cls.objects, cls.bodies = {}, {}
        with tempfile.TemporaryDirectory() as d:
            output = Path(d) / "authored.o"
            for profile, options in (("default", []), ("pooled", ["--r5900-pool-li-d"]),
                                     ("inline", ["--r5900-inline-li-d"])):
                try:
                    result = subprocess.run([str(assembler), *options, "-o", str(output),
                                             str(ROOT / "tests/fixtures/inline_li_d.s")],
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
                    if symbol["name"] in FUNCTIONS:
                        data = obj["sections"][symbol["section"]]["data"]
                        bodies[symbol["name"]] = data[symbol["value"]:symbol["value"] + symbol["size"]]
                cls.objects[profile], cls.bodies[profile] = obj, bodies

    def text_relocations(self, profile):
        obj = self.objects[profile]
        text = next(i for i, s in enumerate(obj["sections"]) if s["name"] == ".text")
        return obj["relocations"].get(text, [])

    def test_the_default_pools_nothing(self):
        self.assertEqual(self.text_relocations("default"), [])
        self.assertEqual(self.bodies["default"], self.bodies["inline"])

    def test_the_sdk_opt_out_still_pools(self):
        # The SDK objects' form: lui $at plus ld from a literal pool for values without zero
        # chunks (all three here).
        self.assertEqual(len(self.bodies["pooled"]["tenth"]), 16)
        self.assertTrue(self.text_relocations("pooled"))
        self.assertEqual(len(self.bodies["pooled"]["simple"]), 16)

    def test_the_retail_form_of_a_tenth(self):
        # func_002ADB78's (double)0.1f argument in $a1.
        self.assertEqual(words(self.bodies["default"]["tenth"]),
                         [0x3405FEE6, 0x00052C78, 0x34A5CCCD, 0x00052F78, 0x03E00008, 0])

    def test_each_chain_builds_its_value(self):
        for name, reg in (("tenth", 5), ("ten4", 18), ("simple", 5)):
            with self.subTest(function=name):
                code = words(self.bodies["default"][name])
                self.assertEqual(code[-2:], [0x03E00008, 0])
                self.assertEqual(evaluate(code[:-2], reg), VALUES[name])


class BuildProfileTests(unittest.TestCase):
    """tools/build.py hands --r5900-pool-li-d to SDK units only."""

    def test_sdk_units_pool(self):
        import build
        _, assembler = build.split_flags("sdk -O2 -G2 -ffunction-sections -Wa,--r5900-sign-bit=unsigned")
        self.assertEqual(assembler, ["--r5900-pool-li-d", "--r5900-sign-bit=unsigned"])

    def test_game_units_take_the_default(self):
        import build
        compiler, assembler = build.split_flags("-O2 -G16 -Wa,-G16 -ffunction-sections")
        self.assertEqual(assembler, ["-G16"])
        self.assertEqual(compiler, ["-O2", "-G16", "-ffunction-sections"])


if __name__ == "__main__":
    unittest.main()
