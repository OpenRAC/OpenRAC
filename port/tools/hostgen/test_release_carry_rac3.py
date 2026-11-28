#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Tests rac3 configuration merging, native pad carry and per-program function identities.

Run with python3 -m unittest discover -s port/tools/hostgen -p test_release_carry_rac3.py.
The syntax test needs Clang, not private disc or generated game inputs.
"""
import contextlib
import io
import json
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent))
from release_carry_rac3 import merge  # noqa: E402
import release_carry_rac3 as carry  # noqa: E402
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

    def carried_pad(self, temporary):
        """Returns the real carried source and compiler includes, without private game inputs."""
        port = Path(__file__).resolve().parents[2]
        names = read_names(port / "game/rac3-ntsc/host/rac3_names.tsv")
        text = rename((port / "game/rac3-pal/host/pad.c").read_text(encoding="utf-8"), names)
        source = Path(temporary) / "pad.c"
        source.write_text(text, encoding="utf-8")
        includes = [port, port / "runtime/include", port / "game/common/include",
                    port / "game/rac3-ntsc/host", port / "game/rac3-pal/host"]
        command = [shutil.which("clang"), "-std=gnu11", "-Wall", "-Wextra"]
        for path in includes:
            command += ["-I", str(path)]
        return port, source, command

    @unittest.skipUnless(shutil.which("clang"), "clang not available")
    def test_renamed_pad_declarations_agree(self):
        """Compiles the real carried C so conflicting folded declarations cannot pass silently."""
        with tempfile.TemporaryDirectory() as temporary:
            _, source, command = self.carried_pad(temporary)
            result = subprocess.run([*command, "-fsyntax-only", str(source)],
                                    capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)

    @unittest.skipUnless(shutil.which("clang"), "clang not available")
    def test_native_pad_graph_has_no_unresolved_release_symbols(self):
        """Requires a closed NTSC pad graph and a definition for every native pad binding."""
        with tempfile.TemporaryDirectory() as temporary:
            port, source, command = self.carried_pad(temporary)
            result = subprocess.run([*command, "-E", "-P", str(source)],
                                    capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        unresolved = sorted(set(re.findall(r"\bfunc_PAL_\w+", result.stdout)))
        self.assertEqual(unresolved, [], "unresolved nodes in compiled NTSC pad graph")
        token = r"func_(?:L\d\d_)?[0-9A-F]{8}(?:__\w+)?"
        references = set(re.findall(r"\b" + token + r"\b", result.stdout))
        definitions = set(re.findall(r"\b(?:void|int)\s+(" + token + r")\([^;{}]*\)\s*\{",
                                     result.stdout))
        # Only proved clear/reader bodies belong outside the native host source.
        self.assertEqual(references - definitions, {
            "func_003A2F98", "func_003A3088", "func_0039DB38__frontbin", "func_L01_0045D5C8"})
        rows = [line.split("\t") for line in
                (port / "game/rac3-ntsc/libraries.tsv").read_text().splitlines()]
        bindings = {row[0] for row in rows if len(row) > 3 and row[1] == "pad" and row[3] == "host"}
        self.assertIn("func_003A2E80", bindings, "the real boot initializer must remain native")
        self.assertEqual(bindings, definitions)


class ProgramIdentity(unittest.TestCase):
    """Program entry identity must not overwrite an existing per-program function mapping."""

    def test_boot_and_frontbin_function_mappings_stay_distinct(self):
        """Keeps program classification fixes from rewriting the genuine boot function mapping."""
        port = Path(__file__).resolve().parents[2]
        path = port / "game/rac3-ntsc/release/release_names.json"
        table = json.loads(path.read_text(encoding="utf-8"))
        self.assertEqual(table["names"]["func_00381250"], "func_00381280")
        front = {names["func_00381250"] for names in table["units"].values()
                 if "func_00381250" in names}
        self.assertEqual(front, {"func_0037D200"})


class CarryEncoding(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        root = Path(self.temp.name)
        self.primary = root / "game/rac3-pal"
        self.release = root / "game/rac3-ntsc"
        self.source = root / "decomp"
        self.note = "caf\u00e9 \u0101"
        files = {
            self.primary / "hostgen.json": json.dumps({"result_allow": "checks/result_allow.tsv"}),
            self.primary / "libraries.tsv": "func_00100000\tlibc\tsample\thost\t" + self.note + "\n",
            self.primary / "checks/result_allow.tsv": "# " + self.note + "\n",
            self.primary / "host/options.c": "/* " + self.note + " */\n",
            self.primary / "profile.cpp": "RAC3_ADDR(0x00123456u)\n",
            self.primary / "mp/hostgen.json": json.dumps({"prefix": "mp_", "asm": {"wide": "wide.json"}}),
            self.primary / "mp/libraries.tsv": "func_00100000\tlibc\tsample\thost\t" + self.note + "\n",
            self.release / "release/release.json": json.dumps({"addresses": {
                "0x00123456": {"is": "0x00123458", "why": self.note}}}, ensure_ascii=False),
            self.release / "release/release_names.json": '{"names": {}}',
            self.release / "release/mp_pairs.json": json.dumps({"functions": {
                "func_00100000": {"is": "func_00100004", "why": self.note}}, "addresses": {}},
                ensure_ascii=False),
            self.source / "config/us/map.tsv": "# " + self.note + "\n"
                "func_00100000\tboot_elf\tfunc_00100004\t0\t4\tC\tmatch\n",
            self.source / "config/levels/functions.tsv": "# " + self.note + "\n",
            self.source / "config/us/levels/functions.tsv": "# " + self.note + "\n",
        }
        for path, text in files.items():
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text, encoding="utf-8")
        for name, value in (("PORT", root), ("PRIMARY", self.primary), ("RELEASE", self.release)):
            self.enterContext(mock.patch.object(carry, name, value))
        self.enterContext(contextlib.redirect_stdout(io.StringIO()))
        self.enterContext(contextlib.redirect_stderr(io.StringIO()))

    @contextlib.contextmanager
    def default_encoding(self, default):
        # Simulate a legacy locale and Windows newlines even on a UTF-8-only CI runner.
        read, write = Path.read_text, Path.write_text

        def read_text(path, encoding=None, errors=None):
            return read(path, encoding=encoding or default, errors=errors)

        def write_text(path, data, encoding=None, errors=None, newline=None):
            return write(path, data, encoding=encoding or default, errors=errors,
                         newline="\r\n" if newline is None else newline)

        with mock.patch.object(Path, "read_text", read_text), mock.patch.object(Path, "write_text", write_text):
            yield

    def snapshot(self):
        return {p.relative_to(self.release): p.read_bytes() for p in self.release.rglob("*") if p.is_file()}

    def test_generation_and_check_do_not_depend_on_the_locale(self):
        for encoding in ("utf-8", "cp1252"):
            with self.subTest(encoding=encoding), self.default_encoding(encoding):
                self.assertEqual(carry.main(["--source", str(self.source)]), 0)
                self.assertEqual(carry.main(["--source", str(self.source), "--check"]), 0)
                if encoding == "utf-8":
                    expected = self.snapshot()
                else:
                    self.assertEqual(self.snapshot(), expected)
        self.assertIn(self.note.encode("utf-8"), (self.release / "host/rac3_addresses.h").read_bytes())
        for path in self.release.rglob("*"):
            if path.is_file():
                self.assertNotIn(b"\r\n", path.read_bytes(), str(path))

    def test_check_reports_stale_utf8_output_without_rewriting_it(self):
        with self.default_encoding("utf-8"):
            self.assertEqual(carry.main(["--source", str(self.source)]), 0)
        (self.release / "host/rac3_addresses.h").write_text("/* " + self.note + " stale */\n", encoding="utf-8")
        before = self.snapshot()
        with self.default_encoding("cp1252"):
            self.assertEqual(carry.main(["--source", str(self.source), "--check"]), 1)
        self.assertEqual(self.snapshot(), before)


if __name__ == "__main__":
    unittest.main()
