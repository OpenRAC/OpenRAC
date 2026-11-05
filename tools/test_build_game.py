#!/usr/bin/env python3
import json
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

import build_game


@unittest.skipUnless(shutil.which("curl") and shutil.which("tar"), "needs curl and tar")
class FetchSource(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.dir = Path(self.tmp.name)
        tree = self.dir / "OpenRAC-abc"
        tree.mkdir()
        (tree / "file").write_text("one")
        subprocess.run(["tar", "-czf", "a.tar.gz", "OpenRAC-abc"], cwd=self.dir, check=True)
        self.url = (self.dir / "a.tar.gz").as_uri()
        self.out = self.dir / "out"

    def test_unpacks_without_the_top_folder_and_the_archive(self):
        source = build_game.fetch_source(self.url, "abc", self.out)
        self.assertEqual((source / "file").read_text(), "one")
        self.assertFalse((source / "source.tar.gz").exists())

    def test_keeps_a_whole_source_of_this_commit(self):
        source = build_game.fetch_source(self.url, "abc", self.out)
        (source / "file").write_text("built on")
        build_game.fetch_source("file:///nothing/here", "abc", self.out)
        self.assertEqual((source / "file").read_text(), "built on")

    def test_replaces_another_commit_and_an_unfinished_one(self):
        source = build_game.fetch_source(self.url, "old", self.out)
        (source / "stale").write_text("x")
        build_game.fetch_source(self.url, "abc", self.out)
        self.assertFalse((source / "stale").exists())
        (source / build_game.DONE).unlink()
        (source / "stale").write_text("x")
        build_game.fetch_source(self.url, "abc", self.out)
        self.assertFalse((source / "stale").exists())

    def test_a_failed_download_is_not_taken_for_a_source(self):
        with self.assertRaises(subprocess.CalledProcessError):
            build_game.fetch_source("file:///nothing/here", "abc", self.out)
        self.assertFalse((self.out / build_game.SOURCE / build_game.DONE).exists())


class Program(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.dir = Path(self.tmp.name)

    def test_copies_either_program_name(self):
        for name in ("openrac-x", "openrac-x.exe"):
            build = self.dir / name / "build"
            build.mkdir(parents=True)
            (build / name).write_text("program")
            self.assertEqual(build_game.copy_program(build, "x", self.dir).name, name)

    def test_no_program_is_an_error(self):
        with self.assertRaises(SystemExit):
            build_game.copy_program(self.dir, "x", self.dir)

    def test_clean_up_leaves_only_this_build(self):
        out = self.dir / "program" / "new"
        (out / "src").mkdir(parents=True)
        (out / "openrac-x").write_text("program")
        (self.dir / "program" / "old" / "src").mkdir(parents=True)
        (self.dir / "program" / "note").write_text("kept")
        build_game.clean_up(out / "src", out)
        self.assertEqual(sorted(p.name for p in out.iterdir()), ["openrac-x"])
        self.assertEqual(sorted(p.name for p in out.parent.iterdir()), ["new", "note"])

    def test_clean_up_deletes_a_read_only_file(self):
        out = self.dir / "program" / "new"
        (out / "src").mkdir(parents=True)
        (out / "src" / "object").write_text("git's")
        (out / "src" / "object").chmod(0o444)
        build_game.clean_up(out / "src", out)
        self.assertFalse((out / "src").exists())

    def test_clean_up_touches_no_other_folder(self):
        out = self.dir / "mine" / "new"
        (out / "src").mkdir(parents=True)
        (self.dir / "mine" / "other").mkdir()
        build_game.clean_up(out / "src", out)
        self.assertTrue((self.dir / "mine" / "other").is_dir())
        self.assertFalse((out / "src").exists())

    def test_the_commit_comes_from_the_stamp(self):
        stamp = self.dir / "openrac-build.json"
        stamp.write_text(json.dumps({"commit": "abc"}))
        self.assertEqual(build_game.carried_commit(stamp), "abc")
        stamp.write_text(json.dumps({"commit": None}))
        with self.assertRaises(SystemExit):
            build_game.carried_commit(stamp)
        with self.assertRaises(SystemExit):
            build_game.carried_commit(self.dir / "none")


if __name__ == "__main__":
    unittest.main()
