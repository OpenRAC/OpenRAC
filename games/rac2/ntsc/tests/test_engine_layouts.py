"""Compile the real headers to catch offset drift, including on 64-bit hosts."""
from pathlib import Path
import shutil
import subprocess
import unittest


class EngineLayouts(unittest.TestCase):
    def test_retail_memory_views_have_fixed_widths_and_offsets(self):
        compiler = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
        if compiler is None:
            self.skipTest("a host C compiler is required for the layout checks")
        root = Path(__file__).resolve().parent.parent
        result = subprocess.run([compiler, "-std=c89", "-fsyntax-only", "-I", str(root / "include"),
                                 str(root / "tests/engine_layouts.c")], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
