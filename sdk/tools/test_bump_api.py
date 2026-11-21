# SPDX-License-Identifier: MIT
# Copyright (c) 2026 the OpenRAC contributors
"""bump_api.py: one version, in every place that holds it."""
from __future__ import annotations

import contextlib
import io
import json
import tempfile
import unittest
from pathlib import Path

import bump_api

HEADER = """/* The interface's minor version, which this header describes. It grows when
 * something is added at the end of the table. 1.1 added script mods, 1.2 the
 * keyboard and mouse, and 1.3 the pointer over the game's picture
 * (openrac_input.pointer_*).
 */
#define OPENRAC_MOD_API_MINOR 3
"""


class BumpTest(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        files = {
            bump_api.HEADER: HEADER,
            bump_api.VERSION: "1.3.2\n",
            bump_api.CMAKE: '# x\nset(PACKAGE_VERSION "1.3")\n',
            bump_api.LAUNCHER: "pub const API_MAJOR: u32 = 1;\npub const API_MINOR: u32 = 3;\n",
        }
        for lock in bump_api.LOCKS:
            files[lock] = json.dumps({"about": "x", "version": "1.3.2", "url": bump_api.URL.format(v="1.3.2"),
                                      "sha256": "ab"}, indent=2) + "\n"
        for path, text in files.items():
            (self.root / path).parent.mkdir(parents=True, exist_ok=True)
            (self.root / path).write_text(text)

    def tearDown(self) -> None:
        self.tmp.cleanup()

    def run_tool(self, *args: str) -> int:
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            return bump_api.main(["--root", str(self.root), "--no-regen", *args])

    def test_check(self) -> None:
        self.assertEqual(self.run_tool("--check"), 0)
        (self.root / bump_api.CMAKE).write_text('set(PACKAGE_VERSION "1.2")\n')
        self.assertEqual(bump_api.problems(self.root), [f"{bump_api.CMAKE} says 1.2, {bump_api.VERSION} says 1.3.2"])
        self.assertEqual(self.run_tool("--check"), 1)
        self.assertRaises(SystemExit, bump_api.main, ["--root", str(self.root), "patch"])

    def test_minor(self) -> None:
        self.assertEqual(self.run_tool("minor", "--note", "the graphics context (graphics)"), 0)
        self.assertEqual(bump_api.problems(self.root), [])
        self.assertEqual((self.root / bump_api.VERSION).read_text(), "1.4.0\n")
        header = (self.root / bump_api.HEADER).read_text()
        self.assertIn("#define OPENRAC_MOD_API_MINOR 4\n", header)
        self.assertIn(" * keyboard and mouse, 1.3 the pointer over the game's picture", header)
        self.assertIn("(openrac_input.pointer_*), and 1.4 the graphics context (graphics).\n */\n", header)
        self.assertIn("1.1 added script mods, 1.2 the\n", header)  # earlier lines stay as they were
        lock = json.loads((self.root / bump_api.LOCKS[0]).read_text())
        self.assertEqual((lock["version"], lock["sha256"]), ("1.4.0", ""))
        self.assertIn("sdk-v1.4.0/openrac-mod-sdk-1.4.0.zip", lock["url"])

    def test_minor_needs_a_note(self) -> None:
        self.assertRaises(SystemExit, bump_api.main, ["--root", str(self.root), "--no-regen", "minor"])

    def test_patch_takes_no_note(self) -> None:
        self.assertRaises(SystemExit, bump_api.main, ["--root", str(self.root), "patch", "--note", "x"])
        self.assertEqual((self.root / bump_api.VERSION).read_text(), "1.3.2\n")

    def test_refused_when_the_places_disagree(self) -> None:
        (self.root / bump_api.VERSION).write_text("1.4.0\n")
        self.assertRaises(SystemExit, bump_api.main, ["--root", str(self.root), "patch"])
        self.assertEqual((self.root / bump_api.VERSION).read_text(), "1.4.0\n")

    def test_two_minors_in_a_row(self) -> None:
        self.run_tool("minor", "--note", "a (a_call)")
        self.run_tool("minor", "--note", "b (b_call)")
        header = (self.root / bump_api.HEADER).read_text()
        self.assertIn("#define OPENRAC_MOD_API_MINOR 5", header)
        self.assertIn("1.3 the pointer", header)
        self.assertIn("1.4 a (a_call), and 1.5 b (b_call).", " ".join(header.replace(" * ", " ").split()))
        self.assertEqual(bump_api.problems(self.root), [])

    def test_patch(self) -> None:
        self.assertEqual(self.run_tool("patch"), 0)
        self.assertEqual(bump_api.problems(self.root), [])
        self.assertEqual((self.root / bump_api.VERSION).read_text(), "1.3.3\n")
        self.assertEqual((self.root / bump_api.HEADER).read_text(), HEADER)


class RealRepoTest(unittest.TestCase):
    def test_places_agree(self) -> None:
        self.assertEqual(bump_api.problems(bump_api.ROOT), [])


if __name__ == "__main__":
    unittest.main()
