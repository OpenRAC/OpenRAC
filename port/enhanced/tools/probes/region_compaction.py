# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Compact final dense grids against their complete existing coverage, before bake alignment."""

from array import array
import math
import struct


class GridCoverage:
    """Exact occupied lattice cells, with bounded-memory summed-volume queries."""

    def __init__(self, regions, spacing):
        self.spacing = spacing
        boxes = [self.coordinates(region['bounds']) for region in regions]
        self.lower = [min(box[a] for box in boxes) for a in range(3)]
        self.upper = [max(box[a + 3] for box in boxes) for a in range(3)]
        self.rows = {}
        for box in boxes:
            mask = ((1 << (box[5] - box[2])) - 1) << (box[2] - self.lower[2])
            for x in range(box[0], box[3]):
                for y in range(box[1], box[4]):
                    self.rows[x, y] = self.rows.get((x, y), 0) | mask
        self.cells = sum(row.bit_count() for row in self.rows.values())
        self.shape = [self.upper[a] - self.lower[a] + 1 for a in range(3)]
        self.prefix = None
        # Unusually large or distant grids use sparse row queries instead of a large allocation.
        if math.prod(self.shape) <= 4 * 1024 * 1024:
            self._build_prefix()

    def coordinates(self, bounds):
        """Reject non-lattice bounds rather than changing their coverage through rounding."""
        coordinates = [round(value / self.spacing[a % 3]) for a, value in enumerate(bounds)]
        if any(abs(value / self.spacing[a % 3] - coordinates[a]) > 1e-4
               for a, value in enumerate(bounds)):
            raise ValueError('dense compaction requires aligned grid bounds')
        return coordinates

    def _build_prefix(self):
        """Build integer occupied-cell sums; each subsequent box query reads eight corners."""
        nx, ny, nz = self.shape
        plane = ny * nz
        prefix = array('I', [0]) * (nx * plane)
        for (x, y), mask in self.rows.items():
            at = (x - self.lower[0] + 1) * plane + (y - self.lower[1] + 1) * nz
            for z in range(1, nz):
                prefix[at + z] = (mask >> (z - 1)) & 1
        for x in range(1, nx):
            for y in range(1, ny):
                at = x * plane + y * nz
                running = 0
                for z in range(1, nz):
                    running += prefix[at + z]
                    prefix[at + z] = (running + prefix[at + z - plane]
                                      + prefix[at + z - nz] - prefix[at + z - plane - nz])
        self.prefix = prefix

    def contains(self, bounds):
        """Return true only if the full candidate box was already densely covered."""
        box = self.coordinates(bounds)
        lo = [box[a] - self.lower[a] for a in range(3)]
        hi = [box[a + 3] - self.lower[a] for a in range(3)]
        if any(lo[a] < 0 or hi[a] >= self.shape[a] for a in range(3)):
            return False
        if self.prefix is None:
            mask = ((1 << (hi[2] - lo[2])) - 1) << lo[2]
            return all(self.rows.get((x, y), 0) & mask == mask
                       for x in range(box[0], box[3]) for y in range(box[1], box[4]))
        _, ny, nz = self.shape
        total = 0
        for corner in range(8):
            x, y, z = [hi[a] if corner & (1 << a) else lo[a] for a in range(3)]
            sign = 1 if corner.bit_count() % 2 else -1
            total += sign * self.prefix[(x * ny + y) * nz + z]
        return total == math.prod(hi[a] - lo[a] for a in range(3))


def native_grid_bounds(bounds, spacing):
    """Mirror bounded_probe_grid's float arithmetic, including outward rounding and dimension ceil."""
    def f32(value):
        return struct.unpack('<f', struct.pack('<f', value))[0]
    low, high = [], []
    for axis in range(3):
        step = f32(spacing[axis])
        origin = f32(math.floor(f32(f32(bounds[axis]) / step)) * step)
        end = f32(math.ceil(f32(f32(bounds[axis + 3]) / step)) * step)
        cells = math.ceil((end - origin) / step)
        start = round(origin / step)
        low.append(start)
        high.append(start + cells)
    return low + high


def native_merge_guard(regions, spacing):
    """Preserve source grids even when native float rounding added a cell beyond planned bounds."""
    cache = {tuple(r['bounds']): native_grid_bounds(r['bounds'], spacing) for r in regions}
    coverage = GridCoverage([{'bounds': box} for box in cache.values()], [1, 1, 1])

    def resolved(bounds):
        key = tuple(bounds)
        if key not in cache:
            cache[key] = native_grid_bounds(bounds, spacing)
        return cache[key]

    def contains(bounds, first, second):
        target = resolved(bounds)
        if not coverage.contains(target):
            return False
        return all(all(target[a] <= source[a] and target[a + 3] >= source[a + 3]
                       for a in range(3)) for source in (resolved(first), resolved(second)))

    return contains


def compact_regions(regions, spacing, budget):
    """Merge within the original union; containing both source boxes preserves their fade support."""
    from moby_seeds import merge_regions
    from seed_regions import probe_count
    if not regions:
        return [], {'before': 0, 'after': 0, 'covered_cells': 0}
    coverage = GridCoverage(regions, spacing)
    merged = merge_regions([dict(region) for region in regions], spacing, budget, preserve_height=True,
                           coverage_contains=coverage.contains, merge_preserves=native_merge_guard(regions, spacing))
    for region in merged:
        region.update(spacing=list(spacing), probe_count=probe_count(region['bounds'], spacing))
    report = {'before': len(regions), 'after': len(merged), 'covered_cells': coverage.cells,
              'probes_before': sum(probe_count(r['bounds'], spacing) for r in regions),
              'probes_after': sum(r['probe_count'] for r in merged)}
    return sorted(merged, key=lambda region: region['bounds']), report
