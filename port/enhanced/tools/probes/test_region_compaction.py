# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Exact cell unions and containing source boxes verify final dense-region compaction."""

import copy
import itertools
import random
import unittest

from region_compaction import GridCoverage, compact_regions, native_grid_bounds
from seed_regions import probe_count


def regions(bounds):
    return [{'bounds': box, 'source_mobys': [n], 'source_transitions': [n]}
            for n, box in enumerate(bounds)]


def cells(boxes, spacing):
    """Independent enumeration of unit lattice cells, without summed-volume queries."""
    result = set()
    for region in boxes:
        box = region['bounds']
        result.update(itertools.product(*(range(round(box[a] / spacing[a]),
                                               round(box[a + 3] / spacing[a])) for a in range(3))))
    return result


class RegionCompaction(unittest.TestCase):
    def test_other_regions_can_fill_pair_gaps(self):
        boxes = regions([[0, 0, 0, 4, 2, 2], [0, 0, 0, 2, 4, 2], [2, 2, 0, 4, 4, 2]])
        merged, report = compact_regions(boxes, [1, 1, 1], 4096)
        self.assertEqual(report['before'], 3)
        self.assertEqual(report['after'], 1)
        self.assertEqual(merged[0]['bounds'], [0, 0, 0, 4, 4, 2])
        self.assertEqual(merged[0]['source_mobys'], [0, 1, 2])
        self.assertEqual(merged[0]['source_transitions'], [0, 1, 2])

    def test_uncovered_room_corners_remain_empty(self):
        boxes = regions([[0, 0, 0, 4, 2, 2], [0, 0, 0, 2, 4, 2]])
        merged, _ = compact_regions(boxes, [1, 1, 1], 4096)
        self.assertEqual(len(merged), 2)
        self.assertEqual(cells(merged, [1, 1, 1]), cells(boxes, [1, 1, 1]))

    def test_random_unions_keep_coverage_fade_support_and_budget(self):
        rng = random.Random(4702)
        spacing = [2, 3, 1]
        for _ in range(30):
            bounds = []
            for _ in range(20):
                lo = [rng.randrange(-3, 4) * step for step in spacing]
                hi = [lo[a] + rng.randrange(1, 5) * spacing[a] for a in range(3)]
                bounds.append(lo + hi)
            boxes = regions(bounds)
            original = copy.deepcopy(boxes)
            merged, _ = compact_regions(boxes, spacing, 125)
            self.assertEqual(boxes, original)
            self.assertEqual(cells(merged, spacing), cells(boxes, spacing))
            self.assertEqual(merged, compact_regions(boxes, spacing, 125)[0])
            self.assertTrue(all(probe_count(r['bounds'], spacing) <= 125 for r in merged))
            # Each source retains a containing output box, so its continuous edge fade cannot shrink.
            for source in boxes:
                self.assertTrue(any(all(target['bounds'][a] <= source['bounds'][a]
                                        and target['bounds'][a + 3] >= source['bounds'][a + 3]
                                        for a in range(3)) for target in merged))

    def test_fractional_spacing_preserves_native_grid_union(self):
        spacing = [2.66666675] * 3
        rng = random.Random(4703)
        for _ in range(20):
            boxes = []
            for n in range(12):
                lo = [rng.randrange(10, 25) for _ in range(3)]
                hi = [value + rng.randrange(1, 8) for value in lo]
                boxes.extend(regions([[value * spacing[a % 3] for a, value in enumerate(lo + hi)]]))
            merged, _ = compact_regions(boxes, spacing, 4096)
            before = [{'bounds': native_grid_bounds(r['bounds'], spacing)} for r in boxes]
            after = [{'bounds': native_grid_bounds(r['bounds'], spacing)} for r in merged]
            self.assertEqual(cells(before, [1] * 3), cells(after, [1] * 3))
            for source in before:
                self.assertTrue(any(all(target['bounds'][a] <= source['bounds'][a]
                                        and target['bounds'][a + 3] >= source['bounds'][a + 3]
                                        for a in range(3)) for target in after))

    def test_sparse_row_queries_match_prefix_queries(self):
        boxes = regions([[-2, -1, 0, 2, 3, 2], [0, 0, 0, 4, 2, 3]])
        coverage = GridCoverage(boxes, [1, 1, 1])
        queries = [box['bounds'] for box in boxes] + [[-2, -1, 0, 4, 3, 3], [0, 0, 0, 2, 2, 2]]
        expected = [coverage.contains(box) for box in queries]
        self.assertEqual(expected, [True, True, False, True])
        coverage.prefix = None
        self.assertEqual([coverage.contains(box) for box in queries], expected)

    def test_empty_and_off_lattice_inputs(self):
        self.assertEqual(compact_regions([], [1, 1, 1], 4096)[0], [])
        with self.assertRaisesRegex(ValueError, 'aligned'):
            compact_regions(regions([[.5, 0, 0, 2, 2, 2]]), [1, 1, 1], 4096)


if __name__ == '__main__':
    unittest.main()
