// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Deterministic grid phase search with short clearance rays and winding-independent occupancy.

#include "enhanced/lighting/bake_alignment.h"

#include <algorithm>
#include <cmath>

#include "common/profiler.h"
#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/bake_transport.h"

namespace openrac::lighting::bake_detail {
namespace {

// Small fixed searches keep this preparation bounded relative to transport and integration.
constexpr std::size_t kPreviewProbes = 64;
constexpr float kPhaseFraction = .2f;      // Per-axis step relative to the smallest cell dimension.
constexpr float kClearanceFraction = .2f;  // Reward no clearance beyond this fraction of cell size.
constexpr double kInsidePenalty = 4;       // One inside probe outweighs four fully cleared probes.
constexpr double kNearPenalty = 2;         // Quadratic penalty for insufficient clearance.
constexpr double kImprovement = 1.e-6;     // Ties retain the original phase.

// Fixed paired directions approximate occupancy independently of triangle winding and shading normals.
std::array<std::array<Float3, 2>, 4> occupancy_directions() {
    constexpr float kDiagonal = .577350269f;  // Unit cube diagonals, 1 / sqrt(3).
    const std::array<Float3, 4> forward{
        Float3{1, 0, 0}, Float3{0, 1, 0}, Float3{0, 0, 1}, Float3{kDiagonal, kDiagonal, kDiagonal}
    };
    std::array<std::array<Float3, 2>, 4> pairs{};

    for (std::size_t n = 0; n < forward.size(); ++n) {
        pairs[n] = {forward[n], mul(forward[n], -1)};
    }

    return pairs;
}

// Retains each outer face coordinate, preserving grid bounds without additional probes or atlas rows.
Float3 anchored_offset(const ProbeGrid& grid, std::size_t index, Float3 offset) {
    for (unsigned axis = 0; axis < offset.size(); ++axis) {
        const auto coordinate = index % grid.dimensions[axis];
        index /= grid.dimensions[axis];

        // A boundary probe stays on its original volume face along this axis.
        if (coordinate == 0 || coordinate + 1 == grid.dimensions[axis]) {
            offset[axis] = 0;
        }
    }

    return grid.frame.to_world(offset);
}

// Estimates capped clearance with short rays, including surfaces coincident with the probe position.
float surface_clearance(const Scene& scene, Float3 position, float bias, float cap) {
    float nearest = cap;
    const float margin = 2 * bias;

    for (const auto& pair : occupancy_directions()) {
        for (const auto& direction : pair) {
            const auto hit =
                scene.trace(add(position, mul(direction, -margin)), direction, bias, cap + margin);

            // Misses retain the exact cap, avoiding rounding-driven near-surface counts.
            if (hit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
                nearest = std::min(nearest, std::max(0.f, hit.ray.tfar - margin));
            }
        }
    }

    return nearest;
}

// Scores a deterministic stratified subset, or every probe when count matches the grid size.
AlignmentScore score_phase(
    const Scene& scene,
    const BakeSettings& settings,
    const ProbeVolume& volume,
    Float3 offset,
    std::size_t count
) {
    AlignmentScore result;
    const auto directions = occupancy_directions();
    const float cell = *std::min_element(volume.grid.spacing.begin(), volume.grid.spacing.end());
    const float cap = std::max(kClearanceFraction * cell, settings.ray_bias * 4);

    // Midpoints sample the complete flattened grid without randomness or a stride tied to an axis.
    for (std::size_t n = 0; n < count; ++n) {
        const auto index = (2 * n + 1) * volume.probes.size() / (2 * count);
        const auto position =
            add(volume.position(index), anchored_offset(volume.grid, index, offset));
        const float nearest = surface_clearance(scene, position, settings.ray_bias, cap);
        const double clearance = nearest / cap;
        const double deficit = 1 - clearance;
        const auto placement = classify(scene, position, settings, directions);
        result.near += nearest < cap;
        result.inside += placement.inside;
        result.mean += clearance * clearance - kNearPenalty * deficit * deficit
                       - kInsidePenalty * static_cast<double>(placement.inside);
    }

    result.mean /= static_cast<double>(count);
    return result;
}

// Selects the best of 27 phases on a small preview; the original phase wins score ties.
Float3 preview_phase(const Scene& scene, const BakeSettings& settings, const ProbeVolume& volume) {
    const auto count = std::min(kPreviewProbes, volume.probes.size());
    const float cell = *std::min_element(volume.grid.spacing.begin(), volume.grid.spacing.end());
    // Leave displacement budget for subsequent per-probe relocation, including anisotropic grids.
    const float step = std::min(kPhaseFraction, settings.relocation_max_fraction / 2) * cell;
    Float3 best{};
    double best_score = score_phase(scene, settings, volume, best, count).mean;

    constexpr unsigned kPhasesPerAxis = 3;  // Negative, zero and positive step.
    constexpr unsigned kPhaseCount = kPhasesPerAxis * kPhasesPerAxis * kPhasesPerAxis;

    for (unsigned phase = 0; phase < kPhaseCount; ++phase) {
        unsigned remaining = phase;
        Float3 offset{};

        for (float& component : offset) {
            component = (static_cast<float>(remaining % kPhasesPerAxis) - 1) * step;
            remaining /= kPhasesPerAxis;
        }

        const double score = score_phase(scene, settings, volume, offset, count).mean;

        // A stable strict comparison avoids unnecessary movement in open space.
        if (score > best_score + kImprovement) {
            best_score = score;
            best = offset;
        }
    }

    return best;
}

}  // namespace

GridAlignment align_probe_grid(
    const Scene& scene, const BakeSettings& settings, ProbeVolume& volume
) {
    const profiling::Zone trace("Align probe grid");

    GridAlignment result;

    // No geometry or displacement budget means alignment cannot improve the original placement.
    if (scene.input.triangles.empty() || settings.relocation_max_fraction == 0) {
        return result;
    }

    // A grid consisting entirely of boundary planes has no movable coordinates.
    if (std::all_of(volume.grid.dimensions.begin(), volume.grid.dimensions.end(), [](auto count) {
            return count <= 2;
        })) {
        return result;
    }

    const auto candidate = preview_phase(scene, settings, volume);

    // Avoid two full-grid passes when the preview already prefers the original phase.
    if (candidate == Float3{}) {
        return result;
    }

    result.before = score_phase(scene, settings, volume, {}, volume.probes.size());
    result.after = score_phase(scene, settings, volume, candidate, volume.probes.size());

    // A preview improvement must also improve the complete volume before any position changes.
    if (result.after.mean <= result.before.mean + kImprovement) {
        result.after = result.before;
        return result;
    }

    result.offset = candidate;

    for (std::size_t index = 0; index < volume.probes.size(); ++index) {
        volume.probes[index].offset = anchored_offset(volume.grid, index, candidate);
    }

    scene.check_error();
    return result;
}

}  // namespace openrac::lighting::bake_detail
