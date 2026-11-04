# SPDX-License-Identifier: GPL-3.0-or-later
"""Run with python -m unittest discover -s port/enhanced/tools/baker -p test_lighting_review.py."""
import unittest

from lighting_review import cell_samples, compare_runs, room_suggestions


def report():
    return {
        "height": 2.5,
        "cells": [[0, 0, 0, 3, 0], [2, 0, 0, 4, 0], [4, 0, 0, 5, 1]],
        "regions": [
            {"id": 3, "labels": [1]}, {"id": 4, "labels": []}, {"id": 5, "labels": []},
        ],
        "transitions": [{"id": 7, "light": [1, 2], "ambient": [[3, 6, 12], [0, 0, 0]]}],
        "seeds": [{"transition": 7, "side": 0, "cell": 0, "label": 1, "target": [0, 0, 1]}],
    }


class LightingReviewTest(unittest.TestCase):
    def test_suggest_uses_indoor_references_and_skips_outdoor_cells(self):
        suggestions = room_suggestions(report())
        self.assertEqual([room["region"] for room in suggestions], [4])
        self.assertEqual(suggestions[0]["position"], [2, 0, 1.25])
        self.assertEqual(suggestions[0]["references"][0]["ambient"], [3, 6, 12])

    def test_near_uses_nearest_floor_instead_of_seed(self):
        data = report()
        data["cells"].extend([[10, 0, 0, 3, 0], [20, 0, 0, 3, 0]])
        ranked = room_suggestions(data, include_labelled=True, near=[0, 0, 0])
        self.assertEqual([room["region"] for room in ranked], [3, 4])
        self.assertEqual(ranked[0]["position"], [10, 0, 1.25])

    def test_compare_matches_positions_after_region_ids_change(self):
        first, second = report(), report()
        second["cells"][1][3] = 20
        second["regions"][1] = {"id": 20, "labels": [1]}
        base = {
            "identity": ("rac1-ntsc", 6, "same"),
            "generation": None,
            "regions": 1,
            "dense_bytes": 128,
            "emitters": {},
            "lights": [],
        }
        old = cell_samples(first, [{"bounds": [-1, -1, 1, 1, 1, 2]}])
        new = cell_samples(second, [{"bounds": [1, -1, 1, 3, 1, 2]}])
        result = compare_runs(base | {"samples": old}, base | {"samples": new})
        self.assertEqual(result["low_sky_cells_with_one_label"], [1, 2])
        self.assertEqual(result["dense_bounds"]["newly_covered"], [(2, 0, 0)])
        self.assertEqual(result["dense_bounds"]["lost_coverage"], [(0, 0, 0)])
        with self.assertRaisesRegex(ValueError, "same game, level"):
            compare_runs(base, base | {"identity": ("rac1-pal", 6, "different")})


if __name__ == "__main__":
    unittest.main()
