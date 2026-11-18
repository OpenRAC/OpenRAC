# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Checks which compiler and linker tools/asm_part.py builds a part with."""
import os
import struct
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

    def test_an_installers_zig_comes_after_a_named_one(self):
        zig = self.dir / "zig.exe"
        zig.write_bytes(b"")
        with mock.patch.object(asm_part, "ZIG_PROGRAM", zig):
            with mock.patch.dict(os.environ, self.env, clear=True):
                self.assertEqual(asm_part.tools(), ([str(zig), "clang"], [str(zig), "ld.lld"]))
            with mock.patch.dict(os.environ, {**self.env, "OPENRAC_PART_ZIG": "/host/zig"}, clear=True):
                self.assertEqual(asm_part.tools()[0], ["/host/zig", "clang"])


def coff(symbols: list[tuple[str, int, int]]) -> bytes:
    """A COFF object's header, symbols (name, value, section; all external) and string table."""
    table, strings = b"", b""
    for name, value, section in symbols:
        if len(name) <= 8:
            raw = name.encode().ljust(8, b"\0")
        else:
            raw = bytes(4) + struct.pack("<I", 4 + len(strings))
            strings += name.encode() + b"\0"
        table += raw + struct.pack("<IhHBB", value, section, 0, 2, 0)
    return struct.pack("<HHIIIHH", 0x8664, 0, 0, 20, len(symbols), 0, 0) + table + struct.pack(
        "<I", 4 + len(strings)) + strings


class Exports(unittest.TestCase):
    def test_what_objects_use_and_do_not_define(self):
        with tempfile.TemporaryDirectory() as tmp:
            one, two = Path(tmp) / "one.o", Path(tmp) / "two.o"
            one.write_bytes(coff([("func_00115200", 0, 0), ("__imp_openrac_guest_base", 0, 0), ("mine", 16, 1)]))
            two.write_bytes(coff([("mine", 0, 0), ("short", 0, 0), ("defined_here", 0, 2)]))
            self.assertEqual(asm_part.external_symbols([str(one), str(two)]),
                             ["__imp_openrac_guest_base", "func_00115200", "short"])

    def test_functions_are_exported_by_name_and_data_as_data(self):
        self.assertEqual(asm_part.exports_text(["__imp_openrac_vu0", "func_00115200"]),
                         "EXPORTS\n  func_00115200\n  openrac_vu0 DATA\n")

    def test_a_part_on_windows_is_a_dll(self):
        self.assertEqual(asm_part.part_name("rac2-ntsc"), "libopenrac-rac2-ntsc-part.so")
        self.assertEqual(asm_part.part_name("rac2-ntsc", True), "libopenrac-rac2-ntsc-part.dll")

    def test_a_windows_part_links_nothing_but_the_program(self):
        with tempfile.TemporaryDirectory() as tmp, mock.patch.dict(os.environ, {"OPENRAC_PART_ZIG": "/z/zig"}), \
                mock.patch.object(asm_part.subprocess, "run") as run:
            kit, out = Path(tmp) / "kit", Path(tmp) / "part.dll.tmp"
            asm_part.link_windows("rac2-ntsc", ["a.o"], out, kit)
        dlltool, link = [call.args[0] for call in run.call_args_list]
        self.assertEqual(dlltool[:6], ["/z/zig", "dlltool", "-m", "i386:x86-64", "-D", "openrac-rac2-ntsc.exe"])
        self.assertEqual(link[:2], ["/z/zig", "lld-link"])
        for flag in ("/dll", "/noentry", "/nodefaultlib"):
            self.assertIn(flag, link)


if __name__ == "__main__":
    unittest.main()
