# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Exact native cell unions and inset fade superlevels verify repartitioned dense regions."""

import copy
import itertools
import random
import unittest

from region_compaction import native_grid_bounds
from region_repartition import box_cells, grid_count, repartition_regions


def regions(boxes, spacing=(1, 1, 1)):
    return [{'bounds': [value * spacing[a % 3] for a, value in enumerate(box)],
             'lattice_bounds': list(box), 'source_mobys': [n], 'source_transitions': [n]}
            for n, box in enumerate(boxes)]


def fade_cells(boxes, inset):
    """Independent quarter-cell superlevel enumeration of the shader's minimum-edge fade."""
    return set().union(*(set(itertools.product(*(range(4 * b[a] + inset, 4 * b[a + 3] - inset)
                                               for a in range(3)))) for b in boxes))


class RegionRepartition(unittest.TestCase):
    def test_aprons_keep_original_coverage_at_full_strength(self):
        from region_repartition import add_blend_aprons
        original = regions([[0, 0, 0, 12, 6, 6], [4, 0, 0, 10, 12, 6]])
        padded = add_blend_aprons(original, [1, 1, 1])
        result, _ = repartition_regions(padded, [1, 1, 1], 256)
        before = [r['lattice_bounds'] for r in original]
        after = [r['lattice_bounds'] for r in result]
        self.assertEqual(fade_cells(before, 0), fade_cells(after, 4))
        self.assertTrue(all(grid_count(b) <= 256 for b in after))
        self.assertTrue(fade_cells(after, 0) > fade_cells(before, 0))

    def verify(self, original, result, budget):
        before = [r['lattice_bounds'] for r in original]
        after = [r['lattice_bounds'] for r in result]
        self.assertEqual(fade_cells(before, 0), fade_cells(after, 0))
        # Monotonic smoothstep has the same superlevels as minimum edge distance.
        for inset in (1, 2, 3, 4):
            self.assertEqual(fade_cells(before, inset), fade_cells(after, inset))
        self.assertTrue(all(grid_count(b) <= budget for b in after))

    def test_crossing_boxes_keep_union_and_fades_without_broad_overlap(self):
        original = regions([[0, 0, 0, 12, 6, 6], [4, 0, 0, 10, 12, 6]])
        result, report = repartition_regions(original, [1, 1, 1], 4096)
        self.verify(original, result, 4096)
        self.assertLess(report['overlap_cells_after'], report['overlap_cells_before'])

    def test_budget_splits_keep_two_cell_blend_bands(self):
        original = regions([[0, 0, 0, 24, 6, 6]])
        result, _ = repartition_regions(original, [1, 1, 1], 256)
        self.assertGreater(len(result), 1)
        self.verify(original, result, 256)
        self.assertTrue(all(4 <= min(b[a + 3] - b[a] for a in range(3)) + 1
                            for b in [r['lattice_bounds'] for r in result]))

    def test_random_negative_bounds_and_fractional_spacing_are_deterministic(self):
        rng = random.Random(4704)
        spacing = [2.66666675, 4, 2]
        for _ in range(8):
            boxes = []
            for _ in range(6):
                low = [rng.randrange(-3, 4) for _ in range(3)]
                boxes.append(low + [value + rng.randrange(3, 7) for value in low])
            original = regions(boxes, spacing)
            snapshot = copy.deepcopy(original)
            result, _ = repartition_regions(original, spacing, 256)
            self.verify(original, result, 256)
            self.assertEqual(original, snapshot)
            self.assertEqual(result, repartition_regions(original, spacing, 256)[0])
            for region in result:
                self.assertEqual(region['probe_count'], grid_count(region['lattice_bounds']))

    def test_thin_regions_keep_partial_strength_support(self):
        original = regions([[0, 0, 0, 8, 4, 1], [0, 0, 1, 6, 4, 3]])
        result, report = repartition_regions(original, [1, 1, 1], 256)
        self.verify(original, result, 256)
        self.assertEqual(report['thin_grids_retained'], 2)

    def test_source_evidence_survives_and_native_inputs_are_resolved(self):
        original = regions([[10, 10, 10, 16, 16, 16], [12, 10, 10, 18, 16, 16]])
        result, _ = repartition_regions(original, [1, 1, 1], 4096)
        self.assertEqual(set().union(*(set(r['source_mobys']) for r in result)), {0, 1})
        self.assertEqual(set().union(*(set(r['source_transitions']) for r in result)), {0, 1})
        del original[0]['lattice_bounds']
        self.assertEqual(native_grid_bounds(original[0]['bounds'], [1, 1, 1]), [10, 10, 10, 16, 16, 16])
        self.assertEqual(result, repartition_regions(original, [1, 1, 1], 4096)[0])

    def test_empty_or_unusable_budget(self):
        self.assertEqual(repartition_regions([], [1, 1, 1], 4096)[0], [])
        with self.assertRaises(ValueError):
            repartition_regions(regions([[0, 0, 0, 4, 4, 4]]), [1, 1, 1], 32)


if __name__ == '__main__':
    unittest.main()
