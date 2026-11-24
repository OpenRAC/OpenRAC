#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Tests rac3 release configuration merging and syntax of the renamed native pad declarations.

Run with python3 -m unittest discover -s port/tools/hostgen -p test_release_carry_rac3.py.
The syntax test needs Clang, not private disc or generated game inputs.
"""
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from release_carry_rac3 import merge  # noqa: E402
import release_rac3  # noqa: E402
from release_host import read_names, rename  # noqa: E402


class Merge(unittest.TestCase):
    def test_dicts_merge_and_lists_of_names_add(self):
        carried = {"asm": {"signatures": {"func_a": "int (int)"}, "enabled": True},
                   "from_assembly": ["func_a", "func_b"]}
        over = {"asm": {"signatures": {"func_c": "float (void)"}, "signatures_why": "evidence"},
                "from_assembly": ["func_b", "func_d"], "from_assembly_why": "evidence"}
        self.assertEqual(merge(carried, over), {
            "asm": {"signatures": {"func_a": "int (int)", "func_c": "float (void)"}, "enabled": True},
            "from_assembly": ["func_a", "func_b", "func_d"]})

    def test_values_and_other_lists_replace_and_null_removes(self):
        out = merge({"frame_rate": 50, "places": "x.tsv", "sources": ["src/a", "src/levels"]},
                    {"frame_rate": 60, "places": None, "sources": ["src/a", "src/us"]})
        self.assertEqual(out, {"frame_rate": 60, "sources": ["src/a", "src/us"]})

class FrontGlobals(unittest.TestCase):
    """release_rac3.py: a front-end global names.tsv places for the executable only."""

    def test_hand_value_is_the_front_end_files_own(self):
        # NTSC-U's frontbin has func_003BD030's scene tables 0x3FB0 below PAL's; names.tsv's only
        # row, the boot program's (+0x90), is frontbin's code there.
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp)
            use = "extern int D_0037FBE0[];\nint {}(void) {{ return D_0037FBE0[0]; }}\n"
            files = {
                "config/us/map.tsv": "func_003BD3E8\tboot_elf\tfunc_003BD310\t003BD310\t860\tC\tmatch\n"
                                     "func_003BD030\tfrontbin\tfunc_003B7B50\t003B7B50\t860\tC\tmatch\n",
                "config/us/names.tsv": "boot_elf\tD_0037FBE0\t0037FC70\tpair\n",
                "src/boot_elf/a.c": use.format("func_003BD3E8"),
                "src/frontbin/b.c": use.format("func_003BD030"),
                "src/us/frontbin/c.c": "",
            }
            for name, text in files.items():
                (source / name).parent.mkdir(parents=True, exist_ok=True)
                (source / name).write_text(text)
            cfg = {"sources": ["src/boot_elf", "src/frontbin"], "includes": []}
            before = release_rac3.build(source, cfg)
            self.assertNotIn("D_0037FBE0", before["units"].get("src/frontbin/b.c", {}))
            after = release_rac3.build(source, cfg, front_globals={"D_0037FBE0": 0x0037BC30})
            self.assertEqual(after["names"]["D_0037FBE0"], "D_0037FC70")
            self.assertEqual(after["units"]["src/frontbin/b.c"]["D_0037FBE0"], "D_0037BC30")
            self.assertNotIn("src/boot_elf/a.c", after["units"])


class NativePads(unittest.TestCase):
    """The real host source must compile when release pairs fold two clear names into one."""

    @unittest.skipUnless(shutil.which("clang"), "clang not available")
    def test_renamed_pad_declarations_agree(self):
        """Compiles the real carried C so conflicting folded declarations cannot pass silently."""
        port = Path(__file__).resolve().parents[2]
        names = read_names(port / "game/rac3-ntsc/host/rac3_names.tsv")
        text = rename((port / "game/rac3-pal/host/pad.c").read_text(encoding="utf-8"), names)
        with tempfile.TemporaryDirectory() as temporary:
            source = Path(temporary) / "pad.c"
            source.write_text(text, encoding="utf-8")
            includes = [port, port / "runtime/include", port / "game/common/include",
                        port / "game/rac3-ntsc/host", port / "game/rac3-pal/host"]
            command = [shutil.which("clang"), "-std=gnu11", "-fsyntax-only", "-Wall", "-Wextra"]
            for path in includes:
                command += ["-I", str(path)]
            result = subprocess.run([*command, str(source)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()
