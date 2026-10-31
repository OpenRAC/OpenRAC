# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Partition full-strength dense support, then restore one-cell smooth blend borders."""

import itertools

from region_compaction import native_grid_bounds
from region_boxes import box_cells, cell_count, grid_count, expanded_core
from region_refinement import layout_metrics, refine_blend_junctions


def grow_core(seed, remaining, budget, order):
    """Grow an occupied rectangular core without borrowing cells from an earlier partition."""
    low = list(seed)
    high = [value + 1 for value in seed]
    for axis in order:
        while True:
            candidate = high.copy()
            candidate[axis] += 1
            if grid_count(expanded_core(low, candidate)) > budget:
                break
            face = itertools.product(*(range(high[a], high[a] + 1) if a == axis
                                       else range(low[a], high[a]) for a in range(3)))
            if not all(cell in remaining for cell in face):
                break
            high = candidate
    return high


def partition_cores(cells, budget, seed_order):
    """Choose the largest available rectangular core at each deterministic seed."""
    remaining = cells.copy()
    result = []
    growth_orders = list(itertools.permutations(range(3)))
    for seed in sorted(cells, key=lambda point: tuple(point[a] for a in seed_order)):
        if seed not in remaining:
            continue
        choices = []
        for order in growth_orders:
            high = grow_core(seed, remaining, budget, order)
            bounds = list(seed) + high
            choices.append((cell_count(bounds), -grid_count(expanded_core(seed, high)), high))
        high = max(choices)[2]
        remaining.difference_update(box_cells(list(seed) + high))
        result.append(expanded_core(seed, high))
    return result


def source_evidence(bounds, originals):
    """Keep source IDs from all original grids intersecting a new box's positive volume."""
    sources = {'source_mobys': set(), 'source_transitions': set()}
    for box, region in originals:
        if any(bounds[a] >= box[a + 3] or box[a] >= bounds[a + 3] for a in range(3)):
            continue
        for name in sources:
            sources[name].update(region.get(name, []))
    return {name: sorted(values) for name, values in sources.items()}


def coverage_cores(originals):
    """Collect full-strength cells while retaining grids too thin to inset on every axis."""
    cells = set()
    thin = []
    for bounds, _ in originals:
        if min(bounds[a + 3] - bounds[a] for a in range(3)) <= 2:
            thin.append(bounds)
            continue
        core = [bounds[a] + 1 for a in range(3)] + [bounds[a + 3] - 1 for a in range(3)]
        cells.update(box_cells(core))
    return cells, thin


def choose_partition(cells, thin, budget):
    """Select compact deterministic cores, then reduce crowded blend junctions."""
    choices = []
    for order in itertools.permutations(range(3)):
        partition = partition_cores(cells, budget, order)
        boxes = partition + thin
        choices.append((sum(cell_count(box) for box in boxes), sum(grid_count(box) for box in boxes),
                        len(boxes), order, partition))
    # Begin with the compact layout, then move shared faces under the complete junction cost.
    baseline = sorted(min(choices)[-1])
    boxes = sorted(refine_blend_junctions(baseline, budget, thin) + thin)
    return boxes


def materialise_regions(boxes, originals, spacing, budget):
    """Attach exact lattice endpoints and seed evidence to each final allocation."""
    result = []
    for bounds in boxes:
        if grid_count(bounds) > budget:
            raise ValueError('native thin grid exceeds the per-volume probe budget')
        result.append({'bounds': [value * spacing[a % 3] for a, value in enumerate(bounds)],
                       'lattice_bounds': bounds, 'spacing': list(spacing),
                       'probe_count': grid_count(bounds), **source_evidence(bounds, originals)})
    return result


def add_blend_aprons(regions, spacing):
    """Put the fade outside intended coverage before repartitioning and enforcing probe budgets."""
    result = []
    for region in regions:
        bounds = list(region.get('lattice_bounds', native_grid_bounds(region['bounds'], spacing)))
        bounds = expanded_core(bounds[:3], bounds[3:])
        result.append({**region, 'lattice_bounds': bounds,
                       'bounds': [value * spacing[a % 3] for a, value in enumerate(bounds)],
                       'probe_count': grid_count(bounds)})
    return result


def repartition_report(originals, boxes, thin, budget):
    """Measure the completed layout independently of metadata and world-space conversion."""
    covered = set().union(*(set(box_cells(box)) for box, _ in originals))
    before_metrics = layout_metrics([box for box, _ in originals], budget)
    after_metrics = layout_metrics(boxes, budget)
    report = {'before': len(originals), 'after': len(boxes), 'covered_cells': len(covered),
              'overlap_cells_before': sum(cell_count(box) for box, _ in originals) - len(covered),
              'overlap_cells_after': sum(cell_count(box) for box in boxes) - len(covered),
              'probes_before': sum(grid_count(box) for box, _ in originals),
              'probes_after': sum(grid_count(box) for box in boxes), 'border_cells': 1,
              'thin_grids_retained': len(thin),
              'layout_before': before_metrics, 'layout_after': after_metrics}
    return report


def repartition_regions(regions, spacing, budget):
    """Preserve native coverage and maximum edge influence with disjoint cores and narrow halos.

    A box with at least two cells per axis is its one-cell-inset core dilated by one cell.
    Dilation distributes over unions, so repartitioning that core union preserves both coverage
    and every intermediate fade superlevel. Thin grids remain intact instead of losing support.
    """
    if not regions:
        return [], {'before': 0, 'after': 0, 'overlap_cells_before': 0, 'overlap_cells_after': 0}
    if budget < 64:
        raise ValueError('repartitioning requires a probe budget of at least 64')
    originals = [(list(r.get('lattice_bounds', native_grid_bounds(r['bounds'], spacing))), r)
                 for r in regions]
    cells, thin = coverage_cores(originals)
    boxes = choose_partition(cells, thin, budget)
    result = materialise_regions(boxes, originals, spacing, budget)
    return result, repartition_report(originals, boxes, thin, budget)
