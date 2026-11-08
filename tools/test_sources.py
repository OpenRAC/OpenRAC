# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Tests for tools/sources.py: patches and files applied to a submodule and taken out again, the
patches turned into commits and written back, and this checkout's submodules pinned where game.json
says. The first ones run on a throwaway git repository standing in for a project."""
import contextlib
import io
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

import sources  # noqa: E402

GIT = ["git", "-c", "user.name=t", "-c", "user.email=t@t", "-c", "commit.gpgsign=false"]


def git(repo: Path, *args: str) -> str:
    return subprocess.run([*GIT, "-C", str(repo), *args], check=True, capture_output=True, text=True).stdout.strip()


class PatchFolderTests(unittest.TestCase):
    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.root = Path(tmp.name)
        self.project = self.root / "games/g/v"
        self.project.mkdir(parents=True)
        git(self.project, "init", "-q")
        git(self.project, "config", "user.name", "t")
        git(self.project, "config", "user.email", "t@t")
        (self.project / "a.c").write_text("int a(void);\n")
        (self.project / "b.c").write_text("int b;\n")
        git(self.project, "add", "-A")
        git(self.project, "commit", "-q", "-m", "start")
        self.base = git(self.project, "rev-parse", "HEAD")
        game = {"id": "g", "versions": {"v": {"source": {
            "name": "g-decomp", "repo": "https://example.invalid/g", "branch": "main",
            "commit": self.base, "date": "2026-01-01"}}, "w": {"source": {"shared": "v"}}}}
        (self.root / "games/g/game.json").write_text(json.dumps(game))
        self.patches = self.root / "games/g/v-patches"
        (self.patches / "files/new").mkdir(parents=True)
        (self.patches / "files/new/ours.txt").write_text("OpenRAC's\n")
        (self.project / "a.c").write_text("int a(int x);\n")
        (self.patches / "0001-Give-a-its-argument.patch").write_text(
            "From: t <t@t>\nSubject: Give a its argument\n\n" + git(self.project, "diff", "--binary") + "\n")
        git(self.project, "checkout", "--", "a.c")
        patcher = mock.patch.object(sources, "ROOT", self.root)
        patcher.start()
        self.addCleanup(patcher.stop)
        self.tree = sources.trees(self.root)["g/v"]

    def quiet(self, fn, *args):
        with contextlib.redirect_stdout(io.StringIO()):
            fn(*args)

    def test_only_versions_with_their_own_source(self):
        self.assertEqual(list(sources.trees(self.root)), ["g/v"])

    def test_apply_and_unapply(self):
        self.quiet(sources.do_apply, self.tree)
        self.assertEqual((self.project / "a.c").read_text(), "int a(int x);\n")
        self.assertEqual((self.project / "new/ours.txt").read_text(), "OpenRAC's\n")
        self.quiet(sources.do_apply, self.tree)  # again: the same result, not applied twice
        self.assertEqual((self.project / "a.c").read_text(), "int a(int x);\n")
        sources.do_unapply(self.tree)
        self.assertEqual((self.project / "a.c").read_text(), "int a(void);\n")
        self.assertFalse((self.project / "new/ours.txt").exists())
        self.assertEqual(git(self.project, "status", "--porcelain"), "")

    def test_apply_refuses_changes_of_the_projects_own(self):
        (self.project / "b.c").write_text("int b = 1;\n")
        with self.assertRaises(SystemExit):
            sources.do_apply(self.tree)

    def test_edit_and_save_write_each_commit_as_a_patch(self):
        self.quiet(sources.do_apply, self.tree)
        self.quiet(sources.do_edit, self.tree)
        self.assertEqual(git(self.project, "log", "-1", "--format=%s"), "Give a its argument")
        (self.project / "b.c").write_text("long b;\n")
        git(self.project, "commit", "-q", "-am", "Make b long\n\nThe console's b is 64 bits.")
        self.quiet(sources.do_save, self.tree)
        names = [p.name for p in self.tree.patch_list()]
        self.assertEqual(names, ["0001-give-a-its-argument.patch", "0002-make-b-long.patch"])
        self.assertIn("The console's b is 64 bits.", (self.patches / names[1]).read_text())
        readme = (self.patches / "README.md").read_text()
        self.assertIn("| [0002-make-b-long.patch](0002-make-b-long.patch) | The console's b is 64 bits. |", readme)
        self.assertIn("[`new/ours.txt`](files/new/ours.txt)", readme)
        # Back on the pinned commit with everything applied, and nothing committed inside.
        self.assertEqual(git(self.project, "rev-parse", "HEAD"), self.base)
        self.assertEqual((self.project / "b.c").read_text(), "long b;\n")
        with self.assertRaises(SystemExit):
            sources.do_save(self.tree)  # not being edited any more

    def test_record_brings_the_shared_versions_along(self):
        sources.record(self.tree, self.base)
        game = json.loads((self.root / "games/g/game.json").read_text())
        self.assertEqual(game["versions"]["w"]["source"]["commit"], self.base)


class DecompTreeTests(unittest.TestCase):
    """A game's decompilation (game.json "decomp"), the one tree every version is built from."""

    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.root = Path(tmp.name)
        (self.root / "games/g").mkdir(parents=True)
        source = {"name": "g-decomp", "repo": "https://example.invalid/g", "branch": "main",
                  "commit": "a" * 40, "date": "2026-01-01"}
        game = {"id": "g", "decomp": {"dir": "games/g/decomp", **source}, "versions": {
            "pal": {"dir": "games/g/decomp", "source": {"shared": "decomp", **source}},
            "ntsc": {"dir": "games/g/decomp", "source": {"shared": "decomp", **source}},
            "jp": {"source": {**source, "name": "other", "commit": "b" * 40}},
            "us": {"dir": "games/g/us-decomp", "source": {**source, "name": "third", "commit": "d" * 40}}}}
        (self.root / "games/g/game.json").write_text(json.dumps(game))
        patcher = mock.patch.object(sources, "ROOT", self.root)
        patcher.start()
        self.addCleanup(patcher.stop)

    def test_the_tree_is_the_games_and_its_patches_beside_it(self):
        found = sources.trees(self.root)
        self.assertEqual(list(found), ["g/decomp", "g/jp", "g/us-decomp"])
        self.assertEqual(sources.chosen(["g/jp"])[0].rel, "games/g/jp")
        tree = found["g/decomp"]
        self.assertEqual((tree.rel, tree.patches), ("games/g/decomp", self.root / "games/g/decomp-patches"))
        self.assertEqual(len(tree.sharing()), 2)

    def test_a_version_names_its_tree(self):
        self.assertEqual([t.key for t in sources.chosen(["g/pal", "g/decomp", "g/ntsc"])], ["g/decomp"])

    def test_a_third_party_decomp_is_named_by_its_folder_or_its_version(self):
        tree = sources.trees(self.root)["g/us-decomp"]
        self.assertEqual((tree.rel, tree.patches), ("games/g/us-decomp", self.root / "games/g/us-decomp-patches"))
        self.assertEqual([t.key for t in sources.chosen(["g/us", "g/us-decomp"])], ["g/us-decomp"])

    def test_record_brings_every_version_along(self):
        tree = sources.trees(self.root)["g/decomp"]
        with mock.patch.object(sources.Tree, "git", return_value="2026-02-02"):
            sources.record(tree, "c" * 40)
        game = json.loads((self.root / "games/g/game.json").read_text())
        self.assertEqual([game["decomp"]["commit"], game["versions"]["pal"]["source"]["commit"],
                          game["versions"]["ntsc"]["source"]["commit"], game["versions"]["jp"]["source"]["commit"]],
                         ["c" * 40, "c" * 40, "c" * 40, "b" * 40])


class PinTests(unittest.TestCase):
    def test_each_submodule_is_pinned_where_game_json_says(self):
        for key, tree in sources.trees().items():
            pinned = tree.pinned()
            if pinned is None:
                self.skipTest("not a git checkout with submodules")
            self.assertEqual(pinned, tree.source["commit"], key)


if __name__ == "__main__":
    unittest.main()
