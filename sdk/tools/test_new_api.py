# SPDX-License-Identifier: MIT
# Copyright (c) 2026 the OpenRAC contributors
"""new_api.py: a new call lands in every place, last in the table."""
from __future__ import annotations

import contextlib
import io
import tempfile
import unittest
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


class NewApiTest(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        files = {
            new_api.HEADER: HEADER,
            new_api.MEMBERS: "# member\tarea\tsince\tstatus\nold\tcore\t1.4\tstable\n",
            new_api.INTERNAL: "namespace x {\nint api_old(openrac_mod* mod);\n\n" + NS,
            Path(new_api.AREA_FILE.format(area="world")): "namespace x {\n\n" + NS,
            new_api.MODS: "void fill() {\n    g_api.old = api_old;\n" + new_api.FILL_END + "\n}\n",
        }
        for path, text in files.items():
            (self.root / path).parent.mkdir(parents=True, exist_ok=True)
            (self.root / path).write_text(text)

    def tearDown(self) -> None:
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
        for args in (["nowhere", "x", "--params", "openrac_mod* mod", "--about", "."],
                     ["world", "old", "--params", "openrac_mod* mod", "--about", "."],
                     ["world", "Bad", "--params", "openrac_mod* mod", "--about", "."],
                     ["world", "x", "--params", "int n", "--about", "."],
                     ["camera", "x", "--params", "openrac_mod* mod", "--about", "."]):
            with self.subTest(args=args), self.assertRaises(SystemExit):
                self.add(*args)
        self.assertEqual(self.text(new_api.HEADER), HEADER)


if __name__ == "__main__":
    unittest.main()
