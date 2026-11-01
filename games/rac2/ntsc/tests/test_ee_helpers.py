"""Compile the reviewed EE transfer helpers, including their real memory effects."""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import elf


class HelperDeclarations(unittest.TestCase):
    def test_memory_clobbers_distinguish_the_copy_variants(self):
        text = (ROOT / "include/common.h").read_text()
        for name in ("qcopy", "qzero", "qcopy_nc", "qzero_nc"):
            body = re.search(r"void " + name + r"\([^)]*\)\s*\{(.*?)\n\}", text, re.S).group(1)
            self.assertEqual('"memory"' in body, name not in ("qcopy_nc", "qzero_nc"))
            self.assertNotIn('""', body)
        self.assertIn("16-byte aligned", text)


class HelperCompilerOutput(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = Path(os.environ.get("RAC2_COMPILER_ROOT", ROOT / "build/compiler/tools"))
        if not all((compiler / n).is_file() for n in ("cpp", "cc1", "as")):
            raise unittest.SkipTest("qualified EE compiler unavailable on this host")
        with tempfile.TemporaryDirectory() as d:
            p = Path(d)
            with (p / "source.i").open("wb") as out:
                try:
                    subprocess.run([str(compiler / "cpp"), "-I" + str(ROOT / "include"),
                                    str(ROOT / "tests/fixtures/ee_helpers.c")], stdout=out, check=True)
                except OSError:
                    if "RAC2_COMPILER_ROOT" in os.environ:
                        raise
                    raise unittest.SkipTest("EE compiler not runnable on this host")
            subprocess.run([str(compiler / "cc1"), "-quiet", "-O2", "-G0", "-ffunction-sections",
                            str(p / "source.i"), "-o", str(p / "source.s")], check=True)
            subprocess.run([str(compiler / "as"), "-o", str(p / "source.o"), str(p / "source.s")], check=True)
            obj = elf.read_object(p / "source.o")
            cls.words = {}
            for sym in obj["symbols"]:
                if sym["type"] == 2 and sym["section"] < len(obj["sections"]):
                    data = obj["sections"][sym["section"]]["data"]
                    cls.words[sym["name"]] = [int.from_bytes(data[i:i+4], "little") for i in range(0, len(data), 4)]

    def test_both_copy_variants_use_one_zero_offset_quadword_pair(self):
        for name in ("copy_quad", "copy_quad_nc"):
            with self.subTest(name=name):
                transfers = [w for w in self.words[name] if w >> 26 in (0x1e, 0x1f)]
                self.assertEqual([w >> 26 for w in transfers], [0x1e, 0x1f])
                self.assertEqual([(w >> 16) & 31 for w in transfers], [2, 2])
                self.assertEqual([w & 0xffff for w in transfers], [0, 0])
                self.assertTrue(all(w in (0, 0x03e00008) or w >> 26 in (0x1e, 0x1f)
                                    for w in self.words[name]))
                self.assertFalse(any(w >> 26 == 3 for w in self.words[name]), "helper did not inline")

    def test_clear_uses_architectural_zero_without_a_materialized_zero(self):
        transfers = [w for w in self.words["clear_quad"] if w >> 26 in (0x1e, 0x1f)]
        self.assertEqual(len(transfers), 1)
        self.assertEqual(transfers[0] >> 26, 0x1f)
        self.assertEqual((transfers[0] >> 16) & 31, 0)
        self.assertEqual(transfers[0] & 0xffff, 0)
        self.assertTrue(all(w in (0, 0x03e00008) or w >> 26 == 0x1f
                            for w in self.words["clear_quad"]))

    def test_clobber_preserves_a_real_read_after_the_memory_write(self):
        for name in ("read_after_copy", "read_after_clear"):
            with self.subTest(name=name):
                words = self.words[name]
                store = next(i for i, w in enumerate(words) if w >> 26 == 0x1f)
                self.assertTrue(any(w >> 26 == 0x23 for w in words[store + 1:]),
                                "C read incorrectly reused the earlier constant")


if __name__ == "__main__":
    unittest.main()
