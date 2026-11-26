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
    tests = {
        "vulkan_test_cache_reset": ({"gpu"}, set()),
        "render_equivalence_prepare_vulkan": ({"gpu"}, {"cache"}),
        "probe_gpu_vulkan": ({"gpu"}, {"cache", "baselines"}),
        "gtao_vulkan": ({"gpu"}, {"cache"}),
        "probe_bake": ({"baker"}, set()),
        "cull_coverage": (set(), set()),
    }
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
        return {name: sorted(closure([name]) - {name}) for name in names}

    return inventory


class EnhancedGroups(unittest.TestCase):
    def plan(self, tests, required=frozenset()):
        with tempfile.TemporaryDirectory() as folder, \
                mock.patch.object(ci_port_tests, "REQUIRED", set(required)), \
                mock.patch.object(ci_port_tests, "inventory", fake_inventory(tests)):
            build = Path(folder)
            ci_port_tests.plan(build)
            return {group: set(ci_port_tests.selection_file(build, group).read_text().split())
                    for group in ci_port_tests.GROUPS}

    def test_vulkan_tests_and_their_setups_run_apart_from_the_others(self):
        groups = self.plan(suite())
        self.assertEqual(groups["host"], {"probe_bake", "cull_coverage"})
        self.assertIn("vulkan_test_cache_reset", groups["vulkan"])
        self.assertIn("render_equivalence_prepare_vulkan", groups["vulkan"])
        # Every test runs once.
        self.assertFalse(groups["host"] & groups["vulkan"])
        self.assertEqual(groups["host"] | groups["vulkan"], set(suite()))

    def test_a_host_test_needing_a_vulkan_setup_stops_the_plan(self):
        with self.assertRaisesRegex(SystemExit, "host tests depend on another group"):
            self.plan(suite({"water_host": (set(), {"cache"})}))

    def test_a_missing_required_test_stops_the_plan(self):
        with self.assertRaisesRegex(SystemExit, "sun_shadow_vulkan"):
            self.plan(suite(), {"sun_shadow_vulkan"})


if __name__ == "__main__":
    unittest.main()
