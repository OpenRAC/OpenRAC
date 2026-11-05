# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Check the conservative pull request build gate."""

import subprocess
import unittest
from unittest.mock import patch

from ci_scope import docs_only


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
