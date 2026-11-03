// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Bounded integer boxes and native float allocation parity for automatic region planning.
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

#include "enhanced/tools/baker/planning/region_plan_internal.h"

namespace openrac::lighting::region_plan {
namespace {
// Four million cells match the bounded prefix-query allocation used by the reference planner.
constexpr std::int64_t kMaximumCells = static_cast<std::int64_t>(kMaximumPlanningCells);

// Avoids integer conversion outside the practical lattice domain, including NaN and infinities.
int checked_integer(double value) {
    // Reject invalid coordinates before narrowing to the bounded integer lattice.
    if (!std::isfinite(value) || value < -1'000'000 || value > 1'000'000) {
        throw std::runtime_error("region lattice coordinate exceeds planning limits");
    }

    return static_cast<int>(value);
}

// Multiplication is checked before intermediate products can overflow a signed cell count.
std::int64_t count(const Box& box, int endpoints) {
    std::int64_t result = 1;

    // All three extents must fit before their product can be trusted.
    for (unsigned axis = 0; axis < 3; ++axis) {
        const auto extent = static_cast<std::int64_t>(box[axis + 3]) - box[axis] + endpoints;

        // Reversed or overflowing boxes cannot describe occupied coverage.
        if (extent <= 0 || result > std::numeric_limits<std::int64_t>::max() / extent) {
            throw std::runtime_error("invalid or oversized region box");
        }

        result *= extent;
    }

    return result;
}
}  // namespace

int nearest_integer(double value) {
    const double lower = std::floor(value);
    const double fraction = value - lower;
    // Python round() selects an even endpoint at exact half-integer ties.
    const bool upper = fraction > .5 || (fraction == .5 && std::fmod(lower, 2) != 0);
    return checked_integer(lower + static_cast<int>(upper));
}

std::int64_t cell_count(const Box& box) {
    return count(box, 0);
}

std::int64_t grid_count(const Box& box) {
    return count(box, 1);
}

std::int64_t probe_count(const Bounds& bounds, const CoveragePoint& spacing) {
    Box box{};

    // Endpoint counts retain the reference nearest-even policy on each axis.
    for (unsigned axis = 0; axis < 3; ++axis) {
        box[axis + 3] = nearest_integer((bounds[axis + 3] - bounds[axis]) / spacing[axis]);
    }

    return grid_count(box);
}

Box lattice_box(const Bounds& bounds, const CoveragePoint& spacing) {
    Box box{};

    // Both corners must lie on the same local lattice.
    for (unsigned index = 0; index < box.size(); ++index) {
        const double value = bounds[index] / spacing[index % 3];
        box[index] = nearest_integer(value);

        // Off-lattice input would change the exact occupied-cell union.
        if (std::abs(value - box[index]) > 1e-4) {
            throw std::runtime_error("dense compaction requires aligned grid bounds");
        }
    }

    return box;
}

Box native_box(const Bounds& bounds, const CoveragePoint& spacing) {
    Box box{};

    // Float snapping must match the allocator rather than double world arithmetic.
    for (unsigned axis = 0; axis < 3; ++axis) {
        const float step = static_cast<float>(spacing[axis]);
        const float origin = std::floor(static_cast<float>(bounds[axis]) / step) * step;
        const float end = std::ceil(static_cast<float>(bounds[axis + 3]) / step) * step;
        const double cells = std::ceil((static_cast<double>(end) - origin) / step);
        box[axis] = nearest_integer(static_cast<double>(origin) / step);
        box[axis + 3] = box[axis] + checked_integer(cells);
    }

    return box;
}

Bounds world_box(const Box& box, const CoveragePoint& spacing) {
    Bounds bounds{};

    // Keep world endpoints in double precision until native grid allocation.
    for (unsigned index = 0; index < bounds.size(); ++index) {
        bounds[index] = box[index] * spacing[index % 3];
    }

    return bounds;
}

Box expanded_box(Box box) {
    // Each side needs one cell for smooth blending.
    for (unsigned axis = 0; axis < 3; ++axis) {
        --box[axis];
        ++box[axis + 3];
    }

    return box;
}

std::vector<Cell> box_cells(const Box& box) {
    const auto cells = cell_count(box);

    // Bound temporary cell collections before allocation.
    if (cells > kMaximumCells) {
        throw std::runtime_error("region coverage exceeds four million planning cells");
    }

    std::vector<Cell> result;
    result.reserve(static_cast<std::size_t>(cells));

    // X-major traversal preserves the reference tie order.
    for (int x = box[0]; x < box[3]; ++x) {
        // Y precedes height in stable occupancy ordering.
        for (int y = box[1]; y < box[4]; ++y) {
            // Each half-open cell is visited once.
            for (int z = box[2]; z < box[5]; ++z) {
                result.push_back({x, y, z});
            }
        }
    }

    return result;
}

std::vector<Bounds> split_bounds(const Bounds& bounds, const Settings& settings) {
    // Small allocations need no split or additional blend band.
    if (probe_count(bounds, settings.spacing) <= settings.budget) {
        return {bounds};
    }

    std::array<int, 3> cells{};

    // The longest lattice extent determines the deterministic split axis.
    for (unsigned axis = 0; axis < 3; ++axis) {
        cells[axis] = nearest_integer((bounds[axis + 3] - bounds[axis]) / settings.spacing[axis]);
    }

    const auto axis =
        static_cast<unsigned>(std::max_element(cells.begin(), cells.end()) - cells.begin());

    // A shorter extent cannot shrink while retaining the shared cut band.
    if (cells[axis] < 4) {
        throw std::runtime_error("region probe budget is too small");
    }

    const int half = cells[axis] / 2;
    const double middle = bounds[axis] + half * settings.spacing[axis];
    Bounds left = bounds;
    Bounds right = bounds;
    left[axis + 3] = middle + settings.spacing[axis];
    right[axis] = middle - settings.spacing[axis];
    auto result = split_bounds(left, settings);
    const auto tail = split_bounds(right, settings);

    // Bound pathological recursive subdivision before accumulating more pieces.
    if (result.size() + tail.size() > 16'384) {
        throw std::runtime_error("region splitting exceeds the planning seed limit");
    }

    result.insert(result.end(), tail.begin(), tail.end());
    return result;
}

std::map<Cell, int> contributor_counts(std::span<const Box> boxes) {
    std::map<Cell, int> counts;

    // The union budget applies across all source boxes.
    for (const auto& box : boxes) {
        // Overlapping boxes contribute separately to the blend score.
        for (const auto& cell : box_cells(box)) {
            ++counts[cell];

            // Reject oversized unions before downstream prefix allocation.
            if (counts.size() > static_cast<std::size_t>(kMaximumCells)) {
                throw std::runtime_error("region coverage exceeds four million planning cells");
            }
        }
    }

    return counts;
}

std::int64_t blend_cell_cost(int contributors) {
    const std::int64_t excess = std::max(0, contributors - 2);
    return (contributors > 2 ? 32 : 0) + 8 * excess * excess + std::max(0, contributors - 1);
}

std::int64_t region_cost(unsigned budget) {
    return std::max(64u, budget / 4);
}

Metrics layout_metrics(std::span<const Box> boxes, unsigned budget) {
    Metrics result{
        {"score", region_cost(budget) * static_cast<std::int64_t>(boxes.size())},
        {"crowded_cells", 0},
        {"excess_squared", 0},
        {"peak_overlap", 0},
        {"overlap_cells", 0},
        {"boxes", static_cast<std::int64_t>(boxes.size())},
        {"probes", 0}
    };

    // Penalise three-way overlap more strongly than ordinary two-grid blends.
    for (const auto& [cell, count] : contributor_counts(boxes)) {
        const std::int64_t excess = std::max(0, count - 2);
        result["score"] += blend_cell_cost(count);
        result["crowded_cells"] += count > 2;
        result["excess_squared"] += excess * excess;
        result["peak_overlap"] = std::max(result["peak_overlap"], static_cast<std::int64_t>(count));
        result["overlap_cells"] += std::max(0, count - 1);
    }

    // Endpoint allocations determine memory cost independently of occupied-cell count.
    for (const auto& box : boxes) {
        result["probes"] += grid_count(box);
    }

    return result;
}
}  // namespace openrac::lighting::region_plan
