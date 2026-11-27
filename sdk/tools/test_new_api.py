# SPDX-License-Identifier: MIT
# Copyright (c) 2026 the OpenRAC contributors
"""new_api.py: a new call lands in every place, last in the table."""
from __future__ import annotations

import contextlib
import io
import os
import shutil
import subprocess
import tempfile
import unittest
import unittest.mock
from pathlib import Path

import new_api

HEADER = """#define OPENRAC_MOD_API_MINOR 4
typedef struct openrac_api {
    uint32_t size;
    /* Old. Since 1.4. */
    int (*old)(openrac_mod* mod);
} openrac_api;
"""
NS = new_api.NAMESPACE_END + "\n"
CLANG_FORMAT = os.environ.get("CLANG_FORMAT") or shutil.which("clang-format")
STYLE = Path(__file__).resolve().parents[2] / "port" / ".clang-format"


class NewApiTest(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        files = {
            new_api.HEADER: HEADER,
            new_api.MEMBERS: "# member\tarea\tsince\tstatus\nold\tcore\t1.4\tstable\n",
            new_api.INTERNAL: "namespace openrac::mods::detail {\nint api_old(openrac_mod* mod);\n\n" + NS,
            Path(new_api.AREA_FILE.format(area="world")): "namespace openrac::mods::detail {\n\n" + NS,
            new_api.MODS: "void fill() {\n    g_api.old = api_old;\n" + new_api.FILL_END + "\n}\n",
        }
        for path, text in files.items():
            (self.root / path).parent.mkdir(parents=True, exist_ok=True)
            (self.root / path).write_text(text)
        # The last release, sdk-v1.4.0, as a local tag: a new member joins 1.5.
        self.env = unittest.mock.patch.dict(os.environ, {"OPENRAC_SDK_OFFLINE": "1"})
        self.env.start()
        git = ["git", "-C", str(self.root), "-c", "user.name=t", "-c", "user.email=t@t"]
        for args in (["init", "-q"], ["add", "-A"], ["commit", "-qm", "x"], ["tag", "sdk-v1.4.0"]):
            subprocess.run(git + args, check=True, capture_output=True)

    def tearDown(self) -> None:
        self.env.stop()
        self.tmp.cleanup()

    def add(self, *args: str) -> int:
        with contextlib.redirect_stdout(io.StringIO()):
            return new_api.main(["--root", str(self.root), *args])

    def text(self, path: Path | str) -> str:
        return (self.root / path).read_text()

    def test_every_place(self) -> None:
        self.add("world", "moby_glow", "--params", "openrac_mod* mod, uint32_t moby", "--about", "Makes it glow.",
                 "--stable")
        header = self.text(new_api.HEADER)
        self.assertIn("    /* Makes it glow. Since 1.5. */\n    int (*moby_glow)(openrac_mod* mod, uint32_t moby);\n"
                      "} openrac_api;", header)
        self.assertTrue(self.text(new_api.MEMBERS).endswith("moby_glow\tworld\t1.5\tstable\n"))
        self.assertIn("int api_moby_glow(openrac_mod* mod, std::uint32_t moby);\n\n" + NS,
                      self.text(new_api.INTERNAL))
        area = self.text(new_api.AREA_FILE.format(area="world"))
        self.assertIn("static_cast<void>(moby);\n    return OPENRAC_E_UNKNOWN;\n}\n\n" + NS, area)
        self.assertIn("    g_api.moby_glow = api_moby_glow;\n" + new_api.FILL_END, self.text(new_api.MODS))

    def test_two_for_one_minor_keep_their_order(self) -> None:
        self.add("world", "a_call", "--params", "openrac_mod* mod", "--about", "A.", "--stable")
        self.add("world", "b_call", "--returns", "const char*", "--params", "openrac_mod* mod", "--about", "B.")
        header = self.text(new_api.HEADER)
        self.assertLess(header.index("a_call"), header.index("b_call"))
        self.assertTrue(self.text(new_api.MEMBERS).endswith("a_call\tworld\t1.5\tstable\nb_call\tworld\t1.5\texperimental\n"))
        self.assertIn("return nullptr;", self.text(new_api.AREA_FILE.format(area="world")))

    def test_long_signature_on_lines_of_its_own(self) -> None:
        params = "openrac_mod* mod, uint32_t first_long_name, uint32_t second_long_name, float third_long_name"
        self.add("world", "long_call", "--params", params, "--about", "Long.")
        self.assertIn("    int (*long_call)(\n        openrac_mod* mod,\n", self.text(new_api.HEADER))

    def test_refused(self) -> None:
        mod = "openrac_mod* mod"
        for args in (["nowhere", "x", "--params", mod, "--about", "."],
                     ["world", "old", "--params", mod, "--about", "."],
                     ["world", "Bad", "--params", mod, "--about", "."],
                     ["world", "new", "--params", mod, "--about", "."],
                     ["world", "a__b", "--params", mod, "--about", "."],
                     ["world", "x" * 41, "--params", mod, "--about", "."],
                     ["world", "x", "--params", "int n", "--about", "."],
                     ["world", "x", "--params", "openrac_mod* mod, int", "--about", "."],
                     ["world", "x", "--params", "openrac_mod* mod, int a, float a", "--about", "."],
                     ["world", "x", "--params", "openrac_mod* mod, void (*fn)(int)", "--about", "."],
                     ["world", "x", "--returns", "int (*)(void)", "--params", mod, "--about", "."],
                     ["world", "x", "--params", mod, "--about", "Ends the */ comment."],
                     ["world", "x", "--params", mod, "--about", " "],
                     ["camera", "x", "--params", mod, "--about", "."]):
            with self.subTest(args=args), self.assertRaises(SystemExit):
                self.add(*args)
        self.assertEqual(self.text(new_api.HEADER), HEADER)

    def test_refused_late_leaves_every_file(self) -> None:
        (self.root / new_api.MODS).write_text("void fill() {}\n")  # no place for the assignment
        before = {p: self.text(p) for p in (new_api.HEADER, new_api.MEMBERS, new_api.INTERNAL)}
        with self.assertRaises(SystemExit):
            self.add("world", "x", "--params", "openrac_mod* mod", "--about", ".")
        self.assertEqual({p: self.text(p) for p in before}, before)

    def test_spacing_and_arrays(self) -> None:
        self.add("world", "pos", "--returns", "const char *", "--params", "openrac_mod *mod,  float  out[3]",
                 "--about", "Pos.")
        self.assertIn("    const char* (*pos)(openrac_mod* mod, float out[3]);", self.text(new_api.HEADER))
        area = self.text(new_api.AREA_FILE.format(area="world"))
        self.assertIn("static_cast<void>(out);", area)
        self.assertIn("return nullptr;", area)

    @unittest.skipUnless(CLANG_FORMAT and STYLE.is_file(), "needs clang-format")
    def test_port_files_as_clang_format_lays_them_out(self) -> None:
        self.add("world", "short_call", "--params", "openrac_mod* mod", "--about", "S.")
        self.add("world", "medium_call", "--params",
                 "openrac_mod* mod, uint32_t moby, float strength, const char* label, int flags", "--about", "M.")
        self.add("world", "long_call", "--params", "openrac_mod* mod, uint32_t first_long_name, "
                 "uint32_t second_long_name, float third_long_name, const char* fourth_long_name", "--about", "L.")
        shutil.copy(STYLE, self.root / ".clang-format")
        for path in (new_api.INTERNAL, Path(new_api.AREA_FILE.format(area="world")), new_api.MODS):
            text = self.text(path)
            done = subprocess.run([CLANG_FORMAT, f"--assume-filename={self.root / path}"], input=text,
                                  capture_output=True, text=True, cwd=self.root, check=True)
            self.assertEqual(done.stdout, text, path)


if __name__ == "__main__":
    unittest.main()
