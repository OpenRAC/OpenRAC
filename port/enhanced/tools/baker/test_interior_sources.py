# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Synthetic label selection for interior source preparation."""
import unittest

from prepare_interior_lights import source_plan


class InteriorSourcesTest(unittest.TestCase):
    def test_only_unambiguous_low_sky_samples(self):
        report = dict(format=1, game='fixture-test', level=1, source_sha256='local',
                      transitions=[dict(id=7, light=[0, 1], ambient=[[0, 0, 0], [1, 2, 3]])],
                      seeds=[dict(transition=7, side=1, label=0)],
                      regions=[dict(id=0, labels=[0]), dict(id=1, labels=[]),
                               dict(id=2, labels=[0, 1])],
                      cells=[[0, 0, 0, 0, 0], [1, 0, 0, 0, 1],
                             [2, 0, 0, 1, 0], [3, 0, 0, 2, 0]])
        plan = source_plan(report, dict(game='fixture', version='test', level=1))
        self.assertEqual(plan['settings']['max_per_region'], 128)
        self.assertEqual(plan['settings']['max_lights'], 1024)
        self.assertEqual(plan['settings']['separation'], 6)
        self.assertEqual(plan['samples'], [dict(position=[0, 0, 0], region=0,
                                               light_set=1, ambient=[1, 2, 3])])

    def test_wrong_level_rejected(self):
        with self.assertRaises(ValueError):
            source_plan(dict(format=1, game='fixture-test', level=2),
                        dict(game='fixture', version='test', level=1))


if __name__ == '__main__':
    unittest.main()
