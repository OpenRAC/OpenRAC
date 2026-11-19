# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Tests for hostgen.json "executable_extents" (hostgen.read_extents, retail.starts_of): a name the
C only declares inside a function the decompilation defines starts nothing there.

    python3 -m unittest port/tools/hostgen/test_extents.py      (no Clang needed)
"""
from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace

sys.path.insert(1, str(next(p for p in Path(__file__).resolve().parents if (p / "tools/openrac_common").is_dir()) / "tools"))

import hostgen  # noqa: E402
import retail  # noqa: E402

UNIT = """#include "common.h"
extern void func_00100010(void);  /* another release's name, inside func_00100000 */
INCLUDE_ASM("asm/text", func_00100000);

void func_00100040(int a)
{
    func_00100010();
}
LINKER_REMNANT("asm/remnants", func_00100060);
"""


class Extents(unittest.TestCase):
    def read(self, text: str) -> dict[int, int]:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "src").mkdir()
            (root / "src" / "a.c").write_text(text)
            return hostgen.read_extents({"sources": ["src"]}, root)

    def test_definitions_run_to_the_next_one(self):
        self.assertEqual(self.read(UNIT), {0x00100000: 0x40, 0x00100040: 0x20})

    def test_a_declared_name_inside_a_definition_starts_nothing(self):
        program = SimpleNamespace(
            functions={n: SimpleNamespace(unit=None) for n in ("func_00100000", "func_00100010", "func_00100040")},
            executable_extents=self.read(UNIT),
            code_places=lambda sym: [(-1, int(sym[5:], 16))],
        )
        self.assertEqual(sorted(retail.starts_of(program)[-1]), [0x00100000, 0x00100040])

    def test_without_the_key_nothing_changes(self):
        self.assertEqual(hostgen.read_extents(None, Path(".")), {})


if __name__ == "__main__":
    unittest.main()
