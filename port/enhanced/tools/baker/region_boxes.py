# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Integer lattice box operations shared by dense partitioning and junction refinement."""

import itertools
import math


def box_cells(bounds):
    """Enumerate unit lattice cells with half-open bounds, independent of world spacing."""
    return itertools.product(*(range(bounds[a], bounds[a + 3]) for a in range(3)))


def cell_count(bounds):
    """Return the volume of an integer box in unit cells."""
    return math.prod(bounds[a + 3] - bounds[a] for a in range(3))


def grid_count(bounds):
    """Include both endpoints when counting a box's probe lattice."""
    return math.prod(bounds[a + 3] - bounds[a] + 1 for a in range(3))


def expanded_core(low, high):
    """One cell on each side matches the shader's smoothstep(0, 1, edge) fade."""
    return [value - 1 for value in low] + [value + 1 for value in high]
