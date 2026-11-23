// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <algorithm>
#include <cmath>

#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/bake_scene.h"
#include "enhanced/lighting/environment_bake.h"

namespace openrac::lighting {
void extend_environment_influence_coverage(
    const BakeScene& collision,
    std::span<const EnvironmentFloorSample> floors,
    const BakeSettings& settings,
    std::span<EnvironmentInfluence> influences
) {
    using namespace bake_detail;
    const Scene scene(collision);
    float maximum_z = -INFINITY;
    for (const auto& triangle : collision.triangles) {
        for (const auto& p : triangle.vertices) {
            maximum_z = std::max(maximum_z, p[2]);
        }
    }
    const float outdoor_height = settings.traversal.player_height * 12;
    std::vector<Float3> receivers, tops;
    for (const auto& floor : floors) {
        auto p = floor.position;
        p[2] += std::max(.05f, settings.ray_bias * 2);
        const auto roof = scene.trace(
            p, {0, 0, 1}, settings.ray_bias, std::max(outdoor_height, maximum_z - p[2] + 1)
        );
        const float top = roof.hit.geomID != RTC_INVALID_GEOMETRY_ID
                              ? p[2] + roof.ray.tfar
                              : floor.position[2] + outdoor_height;
        p[2] = floor.position[2] + std::min(1.25f, settings.traversal.player_height * .5f);
        receivers.push_back(p);
        p[2] = top;
        tops.push_back(p);
    }
    const auto inside = [](const EnvironmentInfluence& v, Float3 p, float margin = 0) {
        p = v.frame.to_local(p);
        for (unsigned k = 0; k < 3; ++k) {
            if (p[k] < v.minimum[k] + margin || p[k] > v.maximum[k] - margin) {
                return false;
            }
        }
        return true;
    };
    for (auto& region : influences) {
        const auto original = region;
        std::vector<unsigned> support, foreign;
        for (unsigned f = 0; f < floors.size(); ++f) {
            if (inside(original, receivers[f])) {
                if (!floors[f].underwater) {
                    support.push_back(f);
                }
            } else {
                foreign.push_back(f);
            }
        }
        if (support.empty()) {
            continue;
        }
        // Keep the fitted horizontal domain: only lift columns which were
        // already supported, never use the whole island's axis-aligned bounds.
        auto full = original;
        for (auto f : support) {
            const auto p = original.frame.to_local(tops[f]);
            for (unsigned k = 0; k < 3; ++k) {
                full.minimum[k] = std::min(full.minimum[k], p[k] - .75f);
                full.maximum[k] = std::max(full.maximum[k], p[k] + .75f);
            }
        }
        const auto candidate = [&](float fraction) {
            auto v = original;
            for (unsigned k = 0; k < 3; ++k) {
                v.minimum[k] += (full.minimum[k] - original.minimum[k]) * fraction;
                v.maximum[k] += (full.maximum[k] - original.maximum[k]) * fraction;
            }
            return v;
        };
        // Allow a small blend apron but prevent a tilted expansion from
        // engulfing another room/storey. Existing overlap consumes no budget.
        const unsigned allowance = unsigned(support.size() / 20);
        const auto acceptable = [&](const EnvironmentInfluence& v) {
            unsigned extra = 0;
            for (auto f : foreign) {
                if (inside(v, receivers[f], .25f) && ++extra > allowance) {
                    return false;
                }
            }
            return true;
        };
        if (acceptable(full)) {
            region = full;
        } else {
            float lo = 0, hi = 1;
            for (unsigned iteration = 0; iteration < 14; ++iteration) {
                const float mid = (lo + hi) * .5f;
                if (acceptable(candidate(mid))) {
                    lo = mid;
                } else {
                    hi = mid;
                }
            }
            region = candidate(lo);
        }
    }
}
}  // namespace openrac::lighting
