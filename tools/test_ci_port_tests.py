# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""The enhanced test plan (ci_port_tests.py plan) on a stand-in CTest: unittest discover -s tools."""

from pathlib import Path
import tempfile
import unittest
from unittest import mock

import ci_port_tests

TESTS = {"probe_gpu_vulkan", "render_equivalence_prepare_vulkan", "probe_bake", "cull_coverage"}


def plan(required):
    with tempfile.TemporaryDirectory() as folder, \
            mock.patch.object(ci_port_tests, "REQUIRED", required), \
            mock.patch.object(ci_port_tests, "inventory", lambda build: {name: [] for name in TESTS}):
        build = Path(folder)
        ci_port_tests.plan(build)
        return set(ci_port_tests.selection_file(build, "all").read_text().split())


class EnhancedPlan(unittest.TestCase):
    def test_every_test_runs_in_the_one_call(self):
        self.assertEqual(plan({"probe_gpu_vulkan"}), TESTS)

    def test_a_missing_required_test_stops_the_plan(self):
        with self.assertRaisesRegex(SystemExit, "gtao_vulkan"):
            plan({"gtao_vulkan"})


if __name__ == "__main__":
    unittest.main()
