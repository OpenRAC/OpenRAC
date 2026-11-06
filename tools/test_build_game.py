#!/usr/bin/env python3
import json
import subprocess
import tarfile
import tempfile
import unittest
from pathlib import Path
from unittest import mock
from unittest.mock import patch

import build_game


def short_enough(test: unittest.TestCase, temp: Path) -> None:
    """Scale the build's longest path to this computer's temporary folder: macOS's is long enough
    on its own to leave no room under Windows' limit, and these tests build in it."""
    patcher = patch.object(build_game, "LONGEST_BUILD_PATH", build_game.WINDOWS_PATH_LIMIT - len(str(temp)) - 40)
    patcher.start()
    test.addCleanup(patcher.stop)


class BuildDirectory(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.dir = Path(self.tmp.name).resolve()
        self.out = self.dir / ("installation with spaces " * 4).rstrip() / "program" / "abc"
        short_enough(self, self.dir)

    def test_windows_keeps_a_short_installation_path(self):
        with patch.object(build_game.sys, "platform", "win32"):
            self.assertEqual(build_game.build_directory(Path("C:/OR")), Path("C:/OR"))

    def test_other_platforms_keep_the_installation_path(self):
        with patch.object(build_game.sys, "platform", "linux"):
            self.assertEqual(build_game.build_directory(self.out), self.out)

    def test_windows_retries_use_the_same_short_folder_but_other_installs_do_not(self):
        with patch.object(build_game.sys, "platform", "win32"), \
                patch.object(build_game.tempfile, "gettempdir", return_value=str(self.dir)):
            directory = build_game.build_directory(self.out)
            self.assertEqual(build_game.build_directory(self.out), directory)
            self.assertNotEqual(build_game.build_directory(self.out / "other"), directory)
        self.assertTrue(directory.is_dir())
        self.assertLessEqual(len(str(directory)) + build_game.LONGEST_BUILD_PATH,
                             build_game.WINDOWS_PATH_LIMIT)
        self.assertFalse(self.out.exists())

    def test_an_overlong_temp_path_fails_before_fetching_or_building(self):
        with patch.object(build_game.sys, "platform", "win32"), \
                patch.object(build_game.tempfile, "gettempdir", return_value=str(self.out)):
            with self.assertRaisesRegex(SystemExit, "temporary folder's path is too long"):
                build_game.build_directory(self.out)
        self.assertFalse(self.out.exists())


class FetchSource(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.dir = Path(self.tmp.name)
        tree = self.dir / "OpenRAC-abc"
        tree.mkdir()
        (tree / "file").write_text("one")
        with tarfile.open(self.dir / "a.tar.gz", "w:gz") as tar:
            tar.add(tree, "OpenRAC-abc")
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

    def test_needs_no_program_of_the_players(self):
        with mock.patch.object(subprocess, "run", side_effect=FileNotFoundError):
            source = build_game.fetch_source(self.url, "abc", self.out)
        self.assertEqual((source / "file").read_text(), "one")

    def test_a_failed_download_is_not_taken_for_a_source(self):
        with self.assertRaises(OSError):
            build_game.fetch_source("file:///nothing/here", "abc", self.out)
        self.assertFalse((self.out / build_game.SOURCE / build_game.DONE).exists())

    def test_long_install_builds_in_temp_and_copies_back_only_after_success(self):
        # A tiny source archive exercises the real download, unpack, child process and copy.
        tree = self.dir / "OpenRAC-abc"
        (tree / "tools").mkdir()
        (tree / "tools" / "build_port.py").write_text(
            "from pathlib import Path\n"
            "import sys\n"
            "disc = Path(sys.argv[sys.argv.index('--disc') + 1])\n"
            "assert disc.is_absolute() and disc.read_text() == 'test fixture'\n"
            "if Path('fail').exists():\n"
            "    Path('retry').write_text('keep partial build')\n"
            "    sys.exit(7)\n"
            "assert Path('retry').read_text() == 'keep partial build'\n"
            "build = Path('port/build/standalone')\n"
            "build.mkdir(parents=True)\n"
            "(build / 'openrac-x.exe').write_text(str(Path.cwd()))\n",
            encoding="utf-8")
        (tree / "fail").touch()
        with tarfile.open(self.dir / "a.tar.gz", "w:gz") as tar:
            tar.add(tree, "OpenRAC-abc")
        out = self.dir / ("installation with spaces " * 4).rstrip() / "program" / "abc"
        out.mkdir(parents=True)
        (out / "openrac-x.exe").write_text("previous program")
        disc = out.parent.parent / "disc.iso"
        disc.write_text("test fixture")
        argv = ["x", "--disc", str(disc), "--out", str(out), "--source-url", self.url]
        short_enough(self, self.dir.resolve())
        with patch.object(build_game.sys, "platform", "win32"), \
                patch.object(build_game.tempfile, "gettempdir", return_value=str(self.dir)), \
                patch.object(build_game, "carried_commit", return_value="abc"):
            with self.assertRaises(subprocess.CalledProcessError) as failure:
                build_game.main(argv)
            self.assertEqual(failure.exception.returncode, 7)
            directory = build_game.build_directory(out.resolve())
            source = directory / build_game.SOURCE
            self.assertEqual((out / "openrac-x.exe").read_text(), "previous program")
            (source / "fail").unlink()
            self.assertEqual(build_game.main(argv), 0)
        self.assertEqual(Path((out / "openrac-x.exe").read_text()), source)
        self.assertFalse(source.exists())
        self.assertFalse((out / build_game.SOURCE).exists())
        self.assertEqual(disc.read_text(), "test fixture")


class Environment(unittest.TestCase):
    def test_linux_drops_only_the_library_variables(self):
        steam = {"LD_LIBRARY_PATH": "/steam", "LD_PRELOAD": "/overlay.so", "PATH": "/usr/bin"}
        with mock.patch.dict(build_game.os.environ, steam, clear=True), mock.patch.object(build_game.sys, "platform", "linux"):
            build_game.use_the_systems_libraries()
            self.assertEqual(dict(build_game.os.environ), {"PATH": "/usr/bin"})

    def test_other_systems_keep_them(self):
        with mock.patch.dict(build_game.os.environ, {"LD_PRELOAD": "x"}, clear=True), mock.patch.object(build_game.sys, "platform", "win32"):
            build_game.use_the_systems_libraries()
            self.assertEqual(dict(build_game.os.environ), {"LD_PRELOAD": "x"})


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

    def test_the_repository_comes_from_the_stamp(self):
        stamp = self.dir / "openrac-build.json"
        stamp.write_text(json.dumps({"commit": "abc", "repository": "someone/OpenRAC"}))
        self.assertEqual(build_game.carried_repository(stamp), "someone/OpenRAC")
        for bad in ({"commit": "abc"}, {"repository": "../x"}, {"repository": 3}):
            stamp.write_text(json.dumps(bad))
            self.assertEqual(build_game.carried_repository(stamp), build_game.REPOSITORY)
        self.assertEqual(build_game.carried_repository(self.dir / "none"), build_game.REPOSITORY)


if __name__ == "__main__":
    unittest.main()
