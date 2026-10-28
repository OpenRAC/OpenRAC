// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Probe grid construction: preserves native bounds rounding and cell-local visibility distances.

#include "enhanced/tools/probes/probe_grid.h"

#include <cmath>
#include <cstdint>
#include <stdexcept>

#include "enhanced/tools/probes/level_scene.h"

namespace openrac::lighting {
namespace {

// Builds the scene-aligned allocation before the caller validates its final extent.
ProbeGrid scene_probe_grid(
    const BakeSceneExport& scene, Float3 spacing, unsigned resolution, bool validate
) {
    // Allocation arithmetic requires a finite positive step on every axis.
    for (float s : spacing) {
        if (!std::isfinite(s) || s <= 0) {
            throw std::runtime_error("probe spacing must be positive");
        }
    }

    ProbeGrid grid;
    grid.spacing = spacing;
    grid.map_resolution = resolution;
    grid.scene_hash = scene.scene_hash;
    double cell_diagonal_squared = 0;

    for (unsigned axis = 0; axis < 3; ++axis) {
        // One full cell of padding and cell-aligned extents.
        const float step = spacing[axis];
        grid.origin[axis] = std::floor(scene.bounds_min[axis] / step) * step - step;
        const double upper = std::ceil(scene.bounds_max[axis] / step) * step + step;
        const double count = std::ceil((upper - grid.origin[axis]) / step) + 1;

        // Dimensions must fit the supported unsigned grid range of 2 through 65536.
        if (!std::isfinite(count) || count < 2 || count > 65536) {
            throw std::runtime_error("unsupported sparse probe extent");
        }

        grid.dimensions[axis] = static_cast<std::uint32_t>(count);
        cell_diagonal_squared += static_cast<double>(step) * step;
    }

    // Moments describe visibility to this cell's eight probes, not the whole
    // level. Distant sky misses otherwise swamp nearby wall depths at openings.
    // Radiance and occupancy still trace the complete scene independently.
    grid.max_distance = static_cast<float>(1.5 * std::sqrt(cell_diagonal_squared));

    // Local allocations validate after replacing these provisional scene extents.
    if (validate) {
        grid.validate();
    }

    return grid;
}

}  // namespace

ProbeGrid sparse_probe_grid(const BakeSceneExport& scene, float spacing, unsigned resolution) {
    return sparse_probe_grid(scene, Float3{spacing, spacing, spacing}, resolution);
}

ProbeGrid sparse_probe_grid(const BakeSceneExport& scene, Float3 spacing, unsigned resolution) {
    return scene_probe_grid(scene, spacing, resolution, true);
}

ProbeGrid bounded_probe_grid(
    const BakeSceneExport& scene, Float3 minimum, Float3 maximum, float spacing, unsigned resolution
) {
    return bounded_probe_grid(
        scene, minimum, maximum, Float3{spacing, spacing, spacing}, resolution
    );
}

ProbeGrid bounded_probe_grid(
    const BakeSceneExport& scene,
    Float3 minimum,
    Float3 maximum,
    Float3 spacing,
    unsigned resolution
) {
    // Only the bounded allocation needs to fit atlas indexing. A fine local
    // grid must not be rejected because a hypothetical whole-level grid at
    // that spacing would exceed the budget.
    auto grid = scene_probe_grid(scene, spacing, resolution, false);

    // Retain the cell-local moment range; allocation bounds do not clip transport.
    for (unsigned axis = 0; axis < 3; ++axis) {
        // Reversed or nonfinite bounds cannot describe a local allocation.
        if (!std::isfinite(minimum[axis]) || !std::isfinite(maximum[axis])
            || minimum[axis] >= maximum[axis]) {
            throw std::runtime_error("invalid local probe bounds");
        }

        const float step = spacing[axis];
        grid.origin[axis] = std::floor(minimum[axis] / step) * step;
        const double end = std::ceil(maximum[axis] / step) * step;
        const double count = std::ceil((end - grid.origin[axis]) / step) + 1;

        // Dimensions must fit the supported unsigned grid range of 2 through 65536.
        if (!std::isfinite(count) || count < 2 || count > 65536) {
            throw std::runtime_error("unsupported local probe extent");
        }

        grid.dimensions[axis] = static_cast<std::uint32_t>(count);
    }

    grid.validate();
    return grid;
}

ProbeGrid lattice_probe_grid(
    const BakeSceneExport& scene,
    const std::array<float, 6>& cells,
    Float3 spacing,
    unsigned resolution
) {
    auto grid = scene_probe_grid(scene, spacing, resolution, false);
    // Cell indices stay exactly representable as floats before multiplication by world spacing.
    constexpr float kMaximumCellIndex = 1'000'000;
    constexpr float kMaximumDimension = 65'536;

    for (std::size_t axis = 0; axis < spacing.size(); ++axis) {
        const float lower = cells[axis];
        const float upper = cells[axis + spacing.size()];
        const float dimension = upper - lower + 1;

        // Malformed plans must not round a fractional cell or overflow a GPU dimension.
        if (!std::isfinite(lower) || !std::isfinite(upper) || std::abs(lower) > kMaximumCellIndex
            || std::abs(upper) > kMaximumCellIndex || std::floor(lower) != lower
            || std::floor(upper) != upper || dimension < 2 || dimension > kMaximumDimension) {
            throw std::runtime_error("invalid probe lattice bounds");
        }

        grid.origin[axis] = lower * spacing[axis];
        grid.dimensions[axis] = static_cast<std::uint32_t>(dimension);
    }

    grid.validate();
    return grid;
}

}  // namespace openrac::lighting
