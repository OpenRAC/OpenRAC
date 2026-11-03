# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Validate restored hostgen output through its real CLI: unittest discover -s tools."""

import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


HOSTGEN = Path(__file__).resolve().parents[1] / "port/tools/hostgen/hostgen.py"


@unittest.skipUnless(shutil.which("clang") or os.environ.get("OPENRAC_CLANG"), "Clang required")
class RestoredOutput(unittest.TestCase):
    def setUp(self):
        """Generate a tiny game, then restore only the independently cached text files."""
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        source = self.root / "decomp"
        (source / "src").mkdir(parents=True)
        (source / "include").mkdir()
        self.unit = source / "src/answer.c"
        self.unit.write_text("int answer(void) { return 1; }\n")
        game = self.root / "game/cache"
        game.mkdir(parents=True)
        config = game / "hostgen.json"
        config.write_text(json.dumps({"name": "cache", "sources": ["src"], "includes": ["include"]}))
        self.out = self.root / "gen"
        self.command = [sys.executable, str(HOSTGEN), "--game", str(config), "--source", str(source),
                        "--out", str(self.out), "--jobs", "1"]
        self.generate()
        (self.out / "hostgen.stamp").write_text("Ninja's generation stamp")
        saved = self.root / "saved"
        shutil.copytree(self.out, saved, ignore=shutil.ignore_patterns("prep", "hostgen.stamp"))
        self.assertEqual(self.root.resolve(), self.out.resolve().parent)
        shutil.rmtree(self.out)
        shutil.copytree(saved, self.out)
        self.assertFalse((self.out / "prep").exists())
        self.assertFalse((self.out / "hostgen.stamp").exists())

    def generate(self):
        """Run the same fingerprint validation that Ninja invokes after a cache restore."""
        result = subprocess.run(self.command, capture_output=True, text=True, encoding="utf-8")
        self.assertEqual(0, result.returncode, result.stdout + result.stderr)
        return result.stdout

    def contents(self):
        """Fingerprint all restored outputs so a cache hit must leave them byte-identical."""
        return {path.relative_to(self.out).as_posix(): hashlib.sha256(path.read_bytes()).hexdigest()
                for path in self.out.rglob("*") if path.is_file()}

    def test_unchanged_output_is_reused_without_recreating_prepared_sources(self):
        """Restored generated text skips translation after validating its input digest."""
        before = self.contents()
        self.assertIn("is up to date", self.generate())
        self.assertFalse((self.out / "prep").exists())
        self.assertEqual(before, self.contents())

    def test_changed_source_regenerates_restored_output(self):
        """A fallback cache containing an older translation cannot hide a source edit."""
        before = self.contents()
        self.unit.write_text("int answer(void) { return 2; }\n")
        self.assertNotIn("is up to date", self.generate())
        self.assertTrue((self.out / "prep").is_dir())
        after = self.contents()
        self.assertNotEqual(before["inputs.sha256"], after["inputs.sha256"])
        changed = [name for name, digest in before.items()
                   if name.endswith(".c") and after.get(name) != digest]
        self.assertTrue(changed)

    def test_frontend_edits_do_not_regenerate_restored_output(self):
        """Unrelated C++ edits must not discard validated translations of any game."""
        before = self.contents()
        common = self.root / "game/common/frontend"
        common.mkdir(parents=True)
        frontend = common / "interpolation.cpp"
        frontend.write_text("// First frontend version.\n")
        self.assertIn("is up to date", self.generate())
        frontend.write_text("// Changed freecam input.\n")
        self.assertIn("is up to date", self.generate())
        self.assertFalse((self.out / "prep").exists())
        self.assertEqual(before, self.contents())

    def test_shared_library_table_invalidates_restored_output(self):
        """Shared host signatures still participate in every game's fingerprint."""
        table = self.root / "game/common/libraries.tsv"
        table.parent.mkdir(parents=True)
        table.write_text("# Changed shared signatures.\n")
        self.assertNotIn("is up to date", self.generate())
        self.assertTrue((self.out / "prep").is_dir())

    def test_missing_required_output_is_regenerated(self):
        """An incomplete restore must regenerate a missing required output despite a valid digest."""
        missing = self.out / "functions.c"
        expected = missing.read_bytes()
        missing.unlink()
        self.assertNotIn("is up to date", self.generate())
        self.assertEqual(expected, missing.read_bytes())


if __name__ == "__main__":
    unittest.main()
