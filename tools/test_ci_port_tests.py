# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""The enhanced test groups (ci_port_tests.py plan) on a stand-in CTest: unittest discover -s tools."""

from pathlib import Path
import tempfile
import unittest
from unittest import mock

import ci_port_tests

# name: (labels, fixture setups it needs). Setups CTest adds to any selection that needs them.
SETUPS = {"vulkan_test_cache_reset": "cache", "render_equivalence_prepare_vulkan": "baselines"}


def suite(extra=None):
    tests = {name: ({"gpu"}, {"cache"}) for name in ci_port_tests.THIRD_RUNNER}
    tests.update({
        "vulkan_test_cache_reset": ({"gpu"}, set()),
        "render_equivalence_prepare_vulkan": ({"gpu"}, {"cache"}),
        "probe_gpu_vulkan": ({"gpu"}, {"cache", "baselines"}),
        "fsr3_motion": ({"gpu"}, {"cache"}),
        **{name: ({"gpu"}, {"cache", "baselines"}) for name in ci_port_tests.SECOND_RUNNER},
        "probe_bake": ({"baker"}, set()),
        "cull_coverage": (set(), set()),
    })
    tests.update(extra or {})
    return tests


def fake_inventory(tests):
    def closure(names):
        names = set(names)
        for name in list(names):
            names |= {setup for setup, fixture in SETUPS.items() if fixture in tests[name][1]}
        return names

    def inventory(build, *selection):
        if not selection:
            names = set(tests)
        elif selection[0] == "-L":
            names = closure(n for n, (labels, _) in tests.items() if selection[1].strip("^$") in labels)
        else:
            names = closure(Path(selection[1]).read_text().split())
        return {name: [] for name in names}

    return inventory


class EnhancedGroups(unittest.TestCase):
    def plan(self, tests):
        with tempfile.TemporaryDirectory() as folder, \
                mock.patch.object(ci_port_tests, "REQUIRED", set()), \
                mock.patch.object(ci_port_tests, "inventory", fake_inventory(tests)):
            build = Path(folder)
            ci_port_tests.plan(build)
            return {group: set(ci_port_tests.selection_file(build, group).read_text().split())
                    for group in ci_port_tests.GROUPS}

    def test_third_runner_takes_its_tests_off_the_vulkan_runner(self):
        groups = self.plan(suite())
        self.assertEqual(groups["effects"], ci_port_tests.THIRD_RUNNER | {"vulkan_test_cache_reset"})
        self.assertFalse(groups["vulkan"] & ci_port_tests.THIRD_RUNNER)
        self.assertIn("probe_gpu_vulkan", groups["vulkan"])
        self.assertIn("vulkan_test_cache_reset", groups["vulkan"])
        # Every test runs on some runner.
        self.assertEqual(set().union(*groups.values()), set(suite()))

    def test_a_third_runner_test_with_another_fixture_stops_the_plan(self):
        tests = suite({"gtao_vulkan": ({"gpu"}, {"cache", "baselines"})})
        with self.assertRaisesRegex(SystemExit, "render_equivalence_prepare_vulkan"):
            self.plan(tests)


if __name__ == "__main__":
    unittest.main()
