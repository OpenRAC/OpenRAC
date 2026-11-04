# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Synthetic core unions verify larger-box refinement, overlap scoring and exact coverage."""

import copy
import unittest

from region_boxes import box_cells, expanded_core, grid_count
from region_refinement import bridge_cores, cell_cost, layout_metrics, refine_blend_junctions
from test_region_repartition import fade_cells


class RegionRefinement(unittest.TestCase):
    def test_third_volume_has_explicit_and_growing_penalty(self):
        self.assertEqual(cell_cost(0), 0)
        self.assertEqual(cell_cost(1), 0)
        self.assertEqual(cell_cost(2), 1)
        self.assertEqual(cell_cost(3), 32 + 8 + 2)
        self.assertEqual(cell_cost(4), 32 + 8 * 4 + 3)

    def test_face_reshape_preserves_disjoint_core_union(self):
        first, second = [0, 0, 0, 4, 2, 6], [2, 2, 0, 8, 4, 3]
        parts = bridge_cores(first, second, 1)
        expected = set(box_cells(first)) | set(box_cells(second))
        actual = set().union(*(set(box_cells(part)) for part in parts))
        self.assertEqual(actual, expected)
        self.assertEqual(sum(len(set(box_cells(part))) for part in parts), len(actual))

    def test_fragmented_rectangular_core_becomes_one_large_box(self):
        cores = [[0, 0, 0, 4, 1, 4], [0, 1, 0, 4, 2, 1], [0, 1, 1, 4, 2, 4]]
        boxes = [expanded_core(box[:3], box[3:]) for box in cores]
        snapshot = copy.deepcopy(boxes)
        result = refine_blend_junctions(boxes, 4096)
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0], [-1, -1, -1, 5, 3, 5])
        self.assertEqual(boxes, snapshot)
        self.assertLess(layout_metrics(result, 4096)['score'], layout_metrics(boxes, 4096)['score'])
        for inset in range(5):
            self.assertEqual(fade_cells(boxes, inset), fade_cells(result, inset))

    def test_budget_keeps_a_necessary_blend_seam(self):
        boxes = [[-1, -1, -1, 3, 3, 3], [1, -1, -1, 5, 3, 3]]
        result = refine_blend_junctions(boxes, 125)
        self.assertEqual(len(result), 2)
        self.assertTrue(all(grid_count(box) <= 125 for box in result))
        self.assertEqual(fade_cells(boxes, 0), fade_cells(result, 0))

    def test_thin_fixed_grids_are_scored_without_being_modified(self):
        boxes = [[-1, -1, -1, 4, 4, 4], [2, -1, -1, 7, 4, 4]]
        fixed = [[0, 0, 0, 6, 4, 1]]
        snapshot = copy.deepcopy(fixed)
        result = refine_blend_junctions(boxes, 4096, fixed)
        self.assertEqual(fixed, snapshot)
        self.assertLessEqual(layout_metrics(result + fixed, 4096)['score'],
                             layout_metrics(boxes + fixed, 4096)['score'])
        self.assertEqual(result, refine_blend_junctions(boxes, 4096, fixed))


if __name__ == '__main__':
    unittest.main()
