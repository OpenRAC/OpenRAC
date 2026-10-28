// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Probe volume diagnostics: membership follows the player and boxes retain nominal bake coverage.

#include "enhanced/viewer/probe_volume_overlay.h"

#include <array>
#include <cstddef>
#include <utility>

#include "common/log.h"
#include "enhanced/lighting/probe_regions.h"
#include "enhanced/viewer/probe_lighting.h"

namespace openrac::lighting {
namespace {
// Three edge directions at each of eight corners, emitted once per undirected edge.
constexpr std::size_t kCorners = 8;
constexpr std::size_t kVerticesPerBox = 24;
constexpr Float3 kDenseColour{1, 1, 0};
constexpr Float3 kSparseColour{0, 1, 1};

// Appends each nominal grid edge once; per-probe offsets do not change the interpolation domain.
void append_box(std::vector<DebugLineVertex>& vertices, const ProbeGrid& grid, Float3 colour) {
    std::array<Float3, kCorners> corners{};

    // Each corner bit chooses the far endpoint of one world axis.
    for (std::size_t n = 0; n < corners.size(); ++n) {
        corners[n] = grid.origin;

        for (std::size_t axis = 0; axis < grid.origin.size(); ++axis) {
            if ((n & (std::size_t{1} << axis)) != 0) {
                corners[n][axis] +=
                    grid.spacing[axis] * static_cast<float>(grid.dimensions[axis] - 1);
            }
        }
    }

    // Only the low endpoint emits an edge, giving twelve distinct segments.
    for (std::size_t n = 0; n < corners.size(); ++n) {
        for (std::size_t axis = 0; axis < grid.origin.size(); ++axis) {
            const std::size_t bit = std::size_t{1} << axis;

            if ((n & bit) == 0) {
                vertices.push_back({corners[n], colour});
                vertices.push_back({corners[n | bit], colour});
            }
        }
    }
}
}  // namespace

std::vector<int> containing_probe_volumes(
    const ProbeGrid& sparse, std::span<const ProbeGrid> dense, Float3 player
) {
    std::vector<int> indices;

    // Overlapping baked regions all appear, even if the camera currently samples different regions.
    for (std::size_t n = 0; n < dense.size(); ++n) {
        if (probe_region_distance(dense[n], player) == 0) {
            indices.push_back(static_cast<int>(n));
        }
    }

    // The sparse box is useful only where no denser coverage contains the player.
    if (indices.empty() && probe_region_distance(sparse, player) == 0) {
        indices.push_back(-1);
    }

    return indices;
}

std::vector<DebugLineVertex> probe_volume_debug_vertices(
    const ProbeGrid& sparse, std::span<const ProbeGrid> dense, std::span<const int> indices
) {
    std::vector<DebugLineVertex> vertices;
    vertices.reserve(indices.size() * kVerticesPerBox);

    // Membership comes from the verified grid list; -1 denotes the sparse fallback.
    for (const int index : indices) {
        if (index < 0) {
            append_box(vertices, sparse, kSparseColour);
            continue;
        }

        // Ignore stale indices rather than addressing outside a newly loaded level's grids.
        if (static_cast<std::size_t>(index) < dense.size()) {
            append_box(vertices, dense[static_cast<std::size_t>(index)], kDenseColour);
        }
    }

    return vertices;
}

void ProbeLighting::draw_volume_bounds(
    const renderer::FrameInput& input, renderer::RenderState& state, std::optional<Float3> player
) {
    // Missing player snapshots must not substitute the camera or leave the previous box visible.
    if (!player) {
        return;
    }

    auto indices = containing_probe_volumes(m_grid, m_region_grids, *player);

    // Movement inside unchanged volumes reuses the resident line mesh.
    if (indices != m_containing_regions) {
        m_volume_bounds.set_vertices(probe_volume_debug_vertices(m_grid, m_region_grids, indices));
        m_containing_regions = std::move(indices);
    }

    std::string error;

    // A broken diagnostic shader disables the overlay until the level reloads.
    if (!m_volume_bounds_failed && !m_volume_bounds.draw(input, state, error)) {
        log::warn("probe volume bounds disabled: {}", error);
        m_volume_bounds_failed = true;
    }
}
}  // namespace openrac::lighting
