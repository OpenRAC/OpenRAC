# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Reshape adjacent dense cores to favour larger boxes and fewer crowded blend junctions."""

from collections import Counter, defaultdict

from region_boxes import box_cells, expanded_core, grid_count


# Opening a third contributor costs more than ordinary two-volume crossfading.
CROWDED_CELL_COST = 32
EXCESS_OVERLAP_COST = 8
# Bounded sweeps keep a difficult level from spending unbounded time refining its layout.
MAXIMUM_SWEEPS = 12


def cell_cost(count):
    """Penalise any third volume, then grow the penalty quadratically for further contributors."""
    crowded = CROWDED_CELL_COST if count > 2 else 0
    excess = EXCESS_OVERLAP_COST * max(0, count - 2) ** 2
    return crowded + excess + max(0, count - 1)


def box_cost(budget):
    """A quarter of one volume's capacity discourages replacing large cores with small fragments."""
    return max(64, budget // 4)


def layout_metrics(boxes, budget):
    """Measure contributors throughout the coverage, rather than only at box centres."""
    counts = Counter(cell for box in boxes for cell in box_cells(box))
    return {'score': sum(cell_cost(count) for count in counts.values()) + box_cost(budget) * len(boxes),
            'crowded_cells': sum(count > 2 for count in counts.values()),
            'excess_squared': sum(max(0, count - 2) ** 2 for count in counts.values()),
            'peak_overlap': max(counts.values(), default=0),
            'overlap_cells': sum(max(0, count - 1) for count in counts.values()),
            'boxes': len(boxes), 'probes': sum(grid_count(box) for box in boxes)}


def subtract_box(box, cut):
    """Partition box minus cut into disjoint slabs; no part extends outside the original box."""
    inside = [max(box[a], cut[a]) for a in range(3)] + [min(box[a + 3], cut[a + 3]) for a in range(3)]
    if any(inside[a] >= inside[a + 3] for a in range(3)):
        return [box]
    live = box.copy()
    result = []
    for axis in range(3):
        if live[axis] < inside[axis]:
            part = live.copy()
            part[axis + 3] = inside[axis]
            result.append(part)
            live[axis] = inside[axis]
        if live[axis + 3] > inside[axis + 3]:
            part = live.copy()
            part[axis] = inside[axis + 3]
            result.append(part)
            live[axis + 3] = inside[axis + 3]
    return result


def adjacent_cores(active):
    """Find face neighbours with positive shared area; edge and corner contacts cannot be bridged."""
    lower, upper = defaultdict(list), defaultdict(list)
    for index, box in active.items():
        for axis in range(3):
            lower[axis, box[axis]].append(index)
            upper[axis, box[axis + 3]].append(index)
    for axis, plane in sorted(lower):
        for first in upper.get((axis, plane), []):
            for second in lower[axis, plane]:
                if all(max(active[first][a], active[second][a]) < min(active[first][a + 3], active[second][a + 3])
                       for a in range(3) if a != axis):
                    yield first, second, axis


def bridge_cores(first, second, axis):
    """Move a shared face through the common footprint, retaining the exact union of both cores."""
    bridge = [max(first[a], second[a]) for a in range(3)] + [min(first[a + 3], second[a + 3]) for a in range(3)]
    bridge[axis] = min(first[axis], second[axis])
    bridge[axis + 3] = max(first[axis + 3], second[axis + 3])
    return [bridge] + subtract_box(first, bridge) + subtract_box(second, bridge)


class LayoutRefiner:
    """Own a deterministic core layout and its incremental blend-contributor counts."""

    def __init__(self, boxes, budget, fixed):
        self.active = {n: [box[a] + 1 for a in range(3)] + [box[a + 3] - 1 for a in range(3)]
                       for n, box in enumerate(boxes)}
        self.counts = Counter(cell for box in [*boxes, *fixed] for cell in box_cells(box))
        self.budget = budget
        self.next_index = len(self.active)

    def replacement(self, first, second, axis):
        """Return a budget-fitting bridge and its contributor changes, or no candidate."""
        # An earlier accepted reshape invalidates this sweep's saved neighbour pair.
        if first not in self.active or second not in self.active:
            return None
        a, b = self.active[first], self.active[second]
        parts = bridge_cores(a, b, axis)
        expanded = [expanded_core(part[:3], part[3:]) for part in parts]
        if any(grid_count(part) > self.budget for part in expanded):
            return None
        delta = Counter(cell for part in expanded for cell in box_cells(part))
        delta.subtract(cell for part in (expanded_core(a[:3], a[3:]), expanded_core(b[:3], b[3:]))
                       for cell in box_cells(part))
        return parts, delta

    def cost_change(self, parts, delta):
        """Include box capacity and every affected cell, including fixed thin grids."""
        cost = box_cost(self.budget) * (len(parts) - 2)
        return cost + sum(cell_cost(self.counts[cell] + change) - cell_cost(self.counts[cell])
                          for cell, change in delta.items() if change)

    def apply(self, first, second, parts, delta):
        """Replace both neighbours without reusing IDs from this sweep's neighbour snapshot."""
        self.counts.update(delta)
        del self.active[first], self.active[second]
        for part in parts:
            self.active[self.next_index] = part
            self.next_index += 1

    def sweep(self):
        """Visit a stable neighbour snapshot in its original order, accepting strict improvements."""
        changes = 0
        for first, second, axis in list(adjacent_cores(self.active)):
            candidate = self.replacement(first, second, axis)
            if candidate is None:
                continue
            parts, delta = candidate
            if self.cost_change(parts, delta) >= 0:
                continue
            self.apply(first, second, parts, delta)
            changes += 1
        return changes

    def run(self):
        """Stop at convergence or the bounded sweep limit; keep insertion order for stable output."""
        for _ in range(MAXIMUM_SWEEPS):
            if not self.sweep():
                break
        return [expanded_core(box[:3], box[3:]) for box in self.active.values()]


def refine_blend_junctions(boxes, budget, fixed=()):
    """Accept only local face reshapes that strictly improve the complete layout's cost.

    Core interiors remain disjoint. Their exact union and one-cell halos are unchanged, so
    refinement preserves coverage and maximum edge-fade support. Thin fixed grids are scored
    but never reshaped. All edits respect the per-volume probe budget.
    """
    return LayoutRefiner(boxes, budget, fixed).run()
