#!/usr/bin/env python3
"""release_carry_rac3.py's merge of rac3-ntsc's release.json "hostgen" keys over rac3-pal's carried
hostgen.json: dicts merge, a list of function names gets the release's added, another list is replaced, a value replaces, null removes."""
import contextlib
import io
import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent))
from release_carry_rac3 import merge  # noqa: E402
import release_carry_rac3 as carry  # noqa: E402


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
