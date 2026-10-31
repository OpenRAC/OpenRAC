// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// floor_matching.h's sorted spatial queries retain the original exhaustive matching decisions.
#include "enhanced/tools/baker/planning/floor_matching.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <vector>

#include "common/profiler.h"

namespace openrac::lighting {
namespace {
// Indexes floor X coordinates once without moving the original graph vertices.
class FloorIndex {
public:
    // Borrows finite floor positions; later anchor and height-bound updates cannot invalidate the index.
    explicit FloorIndex(std::span<const EnvironmentFloorSample> floors)
        : m_floors(floors),
          m_order(floors.size()) {
        std::iota(m_order.begin(), m_order.end(), 0u);
        // Stable floor IDs break equal-coordinate ties independently of the sorting implementation.
        std::sort(m_order.begin(), m_order.end(), [&](unsigned first, unsigned second) {
            const float a = m_floors[first].position[0], b = m_floors[second].position[0];
            return a < b || (a == b && first < second);
        });
    }

    // Returns only floors in the inclusive X interval; callers retain exact Y/Z and distance tests.
    std::span<const unsigned> candidates(double low, double high) const {
        // Reversed or nonnumeric bounds contain no finite floor positions.
        if (!(low <= high)) {
            return {};
        }

        const auto first =
            std::lower_bound(m_order.begin(), m_order.end(), low, [&](unsigned id, double value) {
                return m_floors[id].position[0] < value;
            });
        const auto last =
            std::upper_bound(first, m_order.end(), high, [&](double value, unsigned id) {
                return value < m_floors[id].position[0];
            });
        return std::span<const unsigned>(first, last);
    }

private:
    std::span<const EnvironmentFloorSample> m_floors;  // Borrowed until matching returns.
    std::vector<unsigned>
        m_order;  // Original floor IDs sorted by X, never graph indices to rewrite.
};

// Selects the same closest supported storey and original-index winner as exhaustive floor order.
void mark_anchor(std::span<EnvironmentFloorSample> floors, const FloorIndex& index, Float3 anchor) {
    constexpr float kMaximumScore = 16;  // Original weighted squared-distance threshold.
    constexpr float kRadius = 4, kHeightTolerance = 1.25f;  // Existing anchor search envelope.
    float best = kMaximumScore;
    unsigned selected = ~0u;

    for (const unsigned id :
         index.candidates(double(anchor[0]) - kRadius, double(anchor[0]) + kRadius)) {
        const auto position = floors[id].position;
        const float dx = position[0] - anchor[0], dy = position[1] - anchor[1];
        const float dz = position[2] - anchor[2];
        const float score = dx * dx + dy * dy + 4 * dz * dz;  // Original vertical weighting.

        // Spatial traversal must preserve strict distance bounds and the first original-index tie.
        if (std::abs(dz) <= kHeightTolerance && score < kMaximumScore
            && (score < best || (score == best && id < selected))) {
            best = score;
            selected = id;
        }
    }

    // Unmatched anchors must not make disconnected roofs or cavities reachable.
    if (selected != ~0u) {
        floors[selected].anchor = true;
    }
}

// Keeps malformed optional anchor entries ignored, matching the established seed reader.
void mark_anchors(
    std::span<EnvironmentFloorSample> floors, const FloorIndex& index, const json::Value& anchors
) {
    for (const auto& seed : anchors.items()) {
        // Only ground-snapped three-coordinate anchors identify a playable storey.
        if (!seed["position"].is_array() || seed["position"].items().size() != 3
            || !seed["ground_z"].is_number()) {
            continue;
        }

        mark_anchor(
            floors,
            index,
            {float(seed["position"][0].number()),
             float(seed["position"][1].number()),
             float(seed["ground_z"].number())}
        );
    }
}

// Tests predecoded bounds with the original double-precision containment tolerance.
bool contains(Float3 point, const std::array<double, 6>& bounds) {
    constexpr double kBoundsTolerance = .001;  // Preserve exterior coverage at authored boundaries.
    for (unsigned axis = 0; axis < point.size(); ++axis) {
        // Every axis must contain the floor, including its original height.
        if (!(point[axis] >= bounds[axis] - kBoundsTolerance
              && point[axis] <= bounds[axis + 3] + kBoundsTolerance)) {
            return false;
        }
    }

    return true;
}

// Visits only candidate floors and decodes each region once instead of once per floor.
void match_bounds(
    std::span<EnvironmentFloorSample> floors, const FloorIndex& index, const json::Value& regions
) {
    constexpr double kBoundsTolerance =
        .001;  // Same inclusive tolerance as the complete bounds test.
    for (const auto& region : regions.items()) {
        const auto& values = region["bounds"];

        // Malformed optional bounds contributed nothing in the exhaustive matcher.
        if (!values.is_array() || values.items().size() != 6) {
            continue;
        }

        std::array<double, 6> bounds{};
        for (unsigned axis = 0; axis < bounds.size(); ++axis) {
            bounds[axis] = values[axis].number();
        }

        for (const unsigned id :
             index.candidates(bounds[0] - kBoundsTolerance, bounds[3] + kBoundsTolerance)) {
            auto& floor = floors[id];

            // The X index is conservative; exact three-dimensional containment remains authoritative.
            if (!contains(floor.position, bounds)) {
                continue;
            }

            const Float2 height{float(bounds[2]), float(bounds[5])};

            // Preserve the union of every original covering region, in original region order.
            if (!floor.height_bounds) {
                floor.height_bounds = height;
            } else {
                (*floor.height_bounds)[0] = std::min((*floor.height_bounds)[0], height[0]);
                (*floor.height_bounds)[1] = std::max((*floor.height_bounds)[1], height[1]);
            }
        }
    }
}
}  // namespace

Result<void> match_floor_evidence(
    std::span<EnvironmentFloorSample> floors, const json::Value& seeds
) {
    try {
        const profiling::Zone timing("Match playable floor evidence");
        // Explicit reflection anchors remain independent of optional GI seeding controls.
        const auto& anchors =
            seeds["floor_anchors"].is_array() ? seeds["floor_anchors"] : seeds["moby_seeds"];

        // Missing evidence cannot establish gameplay-reachable floors.
        if (!anchors.is_array()) {
            return unexpected(Error{"environment placement needs gameplay floor anchors"});
        }

        const FloorIndex index(floors);
        mark_anchors(floors, index, anchors);
        match_bounds(floors, index, seeds["regions"]);
        return {};
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}
}  // namespace openrac::lighting
