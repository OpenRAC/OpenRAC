# SPDX-License-Identifier: GPL-3.0-or-later
"""Compile translated anonymous typedefs using the installed host Clang.

This needs no POSIX runtime harness and also exercises Windows source paths.
"""
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

import hostgen


@unittest.skipUnless(shutil.which("clang"), "needs Clang")
class ClangCompatibility(unittest.TestCase):
    def test_nested_source_and_anonymous_typedefs(self):
        with tempfile.TemporaryDirectory(prefix="hostgen compatibility ") as tmp:
            root = Path(tmp)
            source = root / "source"
            (source / "src/nested").mkdir(parents=True)
            (source / "src/nested/example.c").write_text(
                "typedef struct { int value; } Item;\n"
                "typedef union { Item item; int other; } Payload;\n"
                "typedef enum { VALUE = 7 } Kind;\n"
                "int func_00100000(Item *item, Item *end, Kind kind) {\n"
                "  Payload p; p.item.value = item->value;\n"
                "  return p.item.value + kind + sizeof(Payload) * item->value + (end - item);\n"
                "}\n"
            )
            config = root / "game.json"
            config.write_text(json.dumps({"name": "compatibility", "sources": ["src"],
                                          "includes": [], "defines": []}))
            output = root / "generated"
            self.assertEqual(hostgen.main([
                "--game", str(config), "--source", str(source), "--out", str(output),
                "--jobs", "2", "--quiet",
            ]), 0)
            generated = output / "nested/example.c"
            self.assertIn("func_00100000(", generated.read_text())
            report = json.loads((output / "report.json").read_text())
            self.assertEqual(report["units_unreadable"], [])
            self.assertEqual(report["index_problems"], [])
            runtime = Path(__file__).resolve().parents[2] / "runtime/include"
            compiled = subprocess.run([
                shutil.which("clang"), "-std=gnu11", "-fsyntax-only", "-I", str(runtime),
                "-I", str(output), str(generated),
            ], capture_output=True, text=True)
            self.assertEqual(compiled.returncode, 0, compiled.stderr)


if __name__ == "__main__":
    unittest.main()
