# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Checks which compiler and linker tools/asm_part.py builds a part with."""
import os
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

import asm_part  # noqa: E402

VARIABLES = ("OPENRAC_PART_ZIG", "OPENRAC_NATIVE_LIBRARY_DIR", "OPENRAC_PART_CLANG", "OPENRAC_PART_LLD")


class Tools(unittest.TestCase):
    def setUp(self):
        self.dir = Path(tempfile.mkdtemp())
        self.env = {k: v for k, v in os.environ.items() if k not in VARIABLES}

    def test_the_apks_zig_is_clang_and_lld(self):
        (self.dir / asm_part.ZIG_LIBRARY).write_bytes(b"")
        with mock.patch.dict(os.environ, {**self.env, "OPENRAC_NATIVE_LIBRARY_DIR": str(self.dir)}, clear=True):
            clang, lld = asm_part.tools()
        zig = str(self.dir / asm_part.ZIG_LIBRARY)
        self.assertEqual((clang, lld), ([zig, "clang"], [zig, "ld.lld"]))

    def test_a_named_zig_comes_first(self):
        (self.dir / asm_part.ZIG_LIBRARY).write_bytes(b"")
        with mock.patch.dict(os.environ, {**self.env, "OPENRAC_PART_ZIG": "/host/zig",
                                          "OPENRAC_NATIVE_LIBRARY_DIR": str(self.dir)}, clear=True):
            self.assertEqual(asm_part.tools()[1], ["/host/zig", "ld.lld"])

    def test_else_a_clang_and_an_lld(self):
        with mock.patch.dict(os.environ, {**self.env, "OPENRAC_PART_CLANG": "/x/clang",
                                          "OPENRAC_PART_LLD": "/x/lld"}, clear=True):
            self.assertEqual(asm_part.tools(), (["/x/clang"], ["/x/lld", "-flavor", "gnu"]))


if __name__ == "__main__":
    unittest.main()
