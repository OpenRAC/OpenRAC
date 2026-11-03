"""gcse lets every store kill every load (GCC 2.95.3): transformer drift and EE compiler output."""
import importlib.util
import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location("gcse_store_kills_loads",
    ROOT / "tools/compiler/transformers/gcse_store_kills_loads.py")
transformer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(transformer)

FUNCTIONS = ("two_records", "no_store", "across_call")


def function_text(asm: str, name: str) -> str:
    """The function's assembly, its local labels renumbered in order of appearance."""
    body = re.search(r"(?ms)^" + name + r":.*?\.end\s+" + name, asm).group()
    labels = {}
    return re.sub(r"\$L\d+", lambda m: labels.setdefault(m.group(), f"$L{len(labels)}"), body)


def loads(text: str, symbol: str) -> int:
    return len(re.findall(r"\tlw\t\$\d+," + symbol + r"\b", text))


class TransformerTests(unittest.TestCase):
    def test_the_insertion_point_is_used_once(self):
        with tempfile.TemporaryDirectory() as d:
            gcse = Path(d) / "gcse.c"
            gcse.write_text("before\n" + transformer.KILL_OLD + "\nafter\n")
            transformer.patch(gcse)
            text = gcse.read_text()
            self.assertIn("    return;\n\n  /* RAC2:", text)
            self.assertIn("  gcse_mems_conflict_p = 1;\n  return;\n", text)
            self.assertNotIn("flag_rac2", text)
            with self.assertRaisesRegex(ValueError, "mems_conflict_for_gcse_p"):
                transformer.patch(gcse)

    def test_source_drift_is_rejected_without_writing(self):
        with tempfile.TemporaryDirectory() as d:
            gcse = Path(d) / "gcse.c"
            gcse.write_text("unexpected source\n")
            with self.assertRaisesRegex(ValueError, "found 0"):
                transformer.patch(gcse)
            self.assertEqual(gcse.read_text(), "unexpected source\n")


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
            source.write_bytes((ROOT / "tests/fixtures/gcse_store_kills_loads.c").read_bytes())
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

    def test_stores_to_other_records_kill_the_load(self):
        # Alias analysis kept one load of `frame' across the stores to records[12].
        self.assertEqual(loads(self.outputs["two_records"], "frame"), 2)

    def test_without_a_store_the_load_is_reused(self):
        self.assertEqual(loads(self.outputs["no_store"], "frame"), 1)

    def test_a_call_kills_the_load(self):
        self.assertEqual(loads(self.outputs["across_call"], "frame"), 2)


if __name__ == "__main__":
    unittest.main()
