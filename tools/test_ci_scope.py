# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Check the conservative pull request build gate."""

import subprocess
import unittest
from unittest.mock import patch

from ci_scope import docs_only, ios_plan, plan, pushed_paths


class CiScopeTest(unittest.TestCase):
    def test_only_documentation_can_skip_builds(self):
        for diff, expected in (
            (b"docs/port/HDR.md\0docs/SOURCES.md\0", True),
            (b"docs/guide with spaces\nand newline.md\0", True),
            (b"docs/port/DEBUGGING.md\0", False),
            (b"docs/HDR.md\0port/renderer.cpp\0", False),
            (b"docs/HDR.md\0docs/fixture.json\0", False),
            (b"README.md\0", False),
            (b"tools/ci_scope.py\0", False),
            (b"", False),
        ):
            with self.subTest(diff=diff), patch("ci_scope.subprocess.run"), patch(
                "ci_scope.subprocess.check_output", side_effect=[b"base\n", diff]
            ) as output:
                self.assertIs(docs_only("base"), expected)
                self.assertIn("--no-renames", output.call_args.args[0])

    def test_unverified_merge_runs_full_checks(self):
        with patch("ci_scope.subprocess.run", side_effect=subprocess.CalledProcessError(1, "git")):
            self.assertFalse(docs_only("base"))
        with patch("ci_scope.subprocess.run"), patch(
            "ci_scope.subprocess.check_output", return_value=b"other-base\n"
        ) as output:
            self.assertFalse(docs_only("base"))
            output.assert_called_once()


PROGRAMS = {"rac1-pal": False, "rac1-ntsc": False, "rac1-pal1": True, "rac2-ntsc": False,
            "rac2-pal": True, "rac3-ntsc": False}
SHARED = ["common", "rac1-ntsc", "rac1-pal", "rac2-ntsc", "rac3-ntsc"]


class PlanTest(unittest.TestCase):
    def check(self, paths, **expected):
        result = plan(paths, PROGRAMS)
        for key, value in expected.items():
            self.assertEqual(result[key], value, (paths, key))

    def test_a_push_or_an_unclear_diff_runs_everything(self):
        for paths in (None, [".github/workflows/checks.yml"], ["tools/ci_scope.py", "README.md"]):
            self.check(paths, launcher=True, pages=True, port=True, enhanced=True,
                       port_parts=["common", *sorted(PROGRAMS)])

    def test_documentation_runs_no_build(self):
        self.check(["docs/SOURCES.md"], docs_only=True, launcher=False, port=False, port_parts=[])
        self.check(["README.md"], docs_only=False, launcher=False, port=False, port_parts=[])

    def test_markdown_anywhere_runs_no_build(self):
        for path in ("port/docs/CODING_CONVENTIONS.md", "port/enhanced/lighting/README.md",
                     "sdk/docs/hooks.md", "launcher/release-notes/1.0.0.md", "editor/MODDING.md"):
            self.check([path], launcher=False, pages=False, port=False, enhanced=False,
                       port_parts=[])

    def test_a_removed_page_checks_as_before(self):
        # The launcher's test wants every page actions.json links to exist.
        result = plan(["editor/MODDING.md"], PROGRAMS, removed={"editor/MODDING.md"})
        self.assertTrue(result["launcher"])
        result = plan(["port/renderer/README.md"], PROGRAMS, removed={"port/renderer/README.md"})
        self.assertEqual(result["port_parts"], SHARED)

    def test_the_launcher_alone_skips_the_port(self):
        self.check(["launcher/src/App.svelte"], launcher=True, pages=True, port=False,
                   enhanced=False, port_parts=[])
        self.check(["launcher/core/src/lib.rs"], launcher=True, pages=True, port_parts=[])

    def test_one_program_builds_only_itself(self):
        self.check(["port/game/rac2-ntsc/host/x.c"], launcher=False, port=False, enhanced=False,
                   port_parts=["rac2-ntsc"])
        self.check(["port/game/rac2-pal/hostgen.json"], port_parts=["rac2-pal"])

    def test_a_games_shared_folder_builds_its_own_programs(self):
        self.check(["port/game/rac1/host/hand.c"], port=True, enhanced=False,
                   port_parts=["rac1-ntsc", "rac1-pal"])
        self.check(["games/rac2/game.json"], launcher=True, port=False, port_parts=["rac2-ntsc"])

    def test_shared_code_builds_every_program_but_the_carried_releases(self):
        for path in ("port/renderer/frame.cpp", "port/game/common/main.cpp", "tools/build_port.py",
                     "editor/mobys.py"):
            self.check([path], port=True, enhanced=True, port_parts=SHARED)

    def test_a_game_folder_the_shared_tests_compile_also_runs_them(self):
        result = plan(["port/game/rac1-pal/host/destinations.c"], PROGRAMS, {"rac1-pal"})
        self.assertEqual(result["port_parts"], ["common", "rac1-pal"])
        self.assertTrue(result["enhanced"])

    def test_files_the_other_side_reads(self):
        # The port's build reads the launcher's icons and version; the launcher reads the port's
        # bindings and differences; the native env test reads DEBUGGING.md.
        self.check(["launcher/src-tauri/tauri.conf.json"], launcher=True, port=True, port_parts=SHARED)
        self.check(["port/platform/bindings.cpp"], launcher=True, pages=True, port_parts=SHARED)
        self.check(["port/game/rac2-ntsc/differences.json"], launcher=True, pages=True,
                   port_parts=["rac2-ntsc"])
        self.check(["docs/port/DEBUGGING.md"], docs_only=False, launcher=False, port=True,
                   port_parts=SHARED)
        self.check(["games/rac1/game.json"], launcher=True, pages=True)

    def test_the_shared_game_folders_are_found_in_the_checkout(self):
        from pathlib import Path
        from ci_scope import shared_game_dirs
        self.assertIn("rac1-pal", shared_game_dirs(Path(__file__).resolve().parent.parent))


class IosPlanTest(unittest.TestCase):
    def check(self, paths, port, launcher):
        self.assertEqual(ios_plan(paths, {"rac1"}), {"ios_port": port, "ios_launcher": launcher},
                         paths)

    def test_a_push_or_the_workflow_runs_both(self):
        for paths in (None, [".github/workflows/ios.yml"], ["tools/ci_scope.py"]):
            self.check(paths, True, True)

    def test_unrelated_files_run_neither(self):
        for path in ("launcher/src/App.svelte", "port/game/rac2-ntsc/host/x.c", "port/android/x.cpp",
                     "port/tools/hostgen/hostgen.py", "port/renderer/README.md", "editor/mobys.py",
                     "docs/port/IOS.md", ".github/workflows/checks.yml"):
            self.check([path], False, False)

    def test_the_port_side(self):
        for path in ("port/runtime/memory.cpp", "port/game/common/main.cpp", "port/game/rac1/profile.cpp",
                     "port/platform/bindings.cpp", "port/enhanced/lighting/profiles/rac1-pal/a.json",
                     "games/rac1/game.json", "launcher/src-tauri/tauri.conf.json"):
            self.check([path], True, False)

    def test_the_launcher_side(self):
        for path in ("launcher/ios/src/lib.rs", "launcher/core/src/lib.rs", "launcher/Cargo.lock",
                     ".github/actions/launcher-toolchain/action.yml", "launcher/src/lib/palettes.json"):
            self.check([path], False, True)

    def test_files_both_read(self):
        for path in ("port/ios/launcher.mm", "port/platform/ios.h",
                     "port/game/rac2/differences.json", "launcher/src-tauri/icons/icon.png"):
            self.check([path], True, True)

    def test_a_push_is_scoped_by_its_diff(self):
        with patch("ci_scope.subprocess.check_output", return_value=b"port/ios/a.mm\0docs/x.md\0") as output:
            self.assertEqual(pushed_paths("before"), ["port/ios/a.mm", "docs/x.md"])
            self.assertIn("before", output.call_args.args[0])
        with patch("ci_scope.subprocess.check_output", side_effect=subprocess.CalledProcessError(1, "git")):
            self.assertIsNone(pushed_paths("before"))
        with patch("ci_scope.subprocess.check_output", return_value=b""):
            self.assertIsNone(pushed_paths("before"))
