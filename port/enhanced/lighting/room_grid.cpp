// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Fit local GI grids to supported receiver footprints without growing their allocation budget.
#include <algorithm>
#include <limits>

#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/room_layout.h"

namespace openrac::lighting::bake_detail {
namespace {
// Preserve the fitted walls and blend apron while spending spare budget on a narrow cross-section.
ProbeGrid refine_cross_section(
    ProbeGrid best, const ProbeGrid& source, std::span<const Float3> receivers, float clearance
) {
    if (clearance <= 1.5f * source.spacing[0]) {
        Float3 lo{INFINITY, INFINITY, 0}, hi{-INFINITY, -INFINITY, 0};

        for (auto p : receivers) {
            p = best.frame.to_local(p);

            for (unsigned a = 0; a < 2; ++a) {
                lo[a] = std::min(lo[a], p[a]);
                hi[a] = std::max(hi[a], p[a]);
            }
        }

        for (unsigned a = 0; a < 2; ++a) {
            const float width = hi[a] - lo[a], length = hi[1 - a] - lo[1 - a];

            if (width >= 2 * source.spacing[a] || length <= 4 * source.spacing[1 - a]
                || length <= 3 * width) {
                continue;
            }

            auto refined = best;
            refined.spacing[a] *= .5f;
            refined.dimensions[a] = (refined.dimensions[a] - 1) * 2 + 1;

            if (refined.probe_count() <= source.probe_count()) {
                best = refined;
            }
        }
    }

    return best;
}
}  // namespace

ProbeGrid fit_room_grid(
    const ProbeGrid& source,
    const BakeScene& /*scene*/,
    std::span<const EnvironmentFloorSample> floors,
    const RoomLayout& layout,
    bool refine_corridors
) {
    if (floors.empty()) {
        return source;
    }

    Float3 maximum = source.origin, centre = source.origin;

    for (unsigned a = 0; a < 3; ++a) {
        maximum[a] += source.spacing[a] * float(source.dimensions[a] - 1);
        centre[a] = (source.origin[a] + maximum[a]) * .5f;
    }

    auto inside = [&](Float3 p) {
        for (unsigned a = 0; a < 3; ++a) {
            if (p[a] < source.origin[a] - .001f || p[a] > maximum[a] + .001f) {
                return false;
            }
        }

        return true;
    };
    unsigned nearest = 0;
    float distance = std::numeric_limits<float>::infinity();
    std::vector<Float3> receivers;

    for (unsigned a = 0; a < floors.size(); ++a) {
        const auto p = floors[a].position;
        const auto delta = add(p, mul(centre, -1));

        if (dot(delta, delta) < distance) {
            nearest = a;
            distance = dot(delta, delta);
        }

        if (inside(p)) {
            receivers.push_back(p);
        }
    }

    if (receivers.size() < 3) {
        return source;
    }

    // The planner has already extended confirmed interiors to their ceilings. Keep that
    // original vertical envelope exactly; wall vertices and outdoor roofs cannot raise it here.
    auto best = source;
    const auto oriented = layout.frame(floors, layout.owners[nearest], centre);

    for (auto frame : {oriented, ProbeFrame{}}) {
        Float3 lo{INFINITY, INFINITY, source.origin[2]}, hi{-INFINITY, -INFINITY, maximum[2]};

        for (auto p : receivers) {
            p = frame.to_local(p);

            for (unsigned a = 0; a < 2; ++a) {
                lo[a] = std::min(lo[a], p[a]);
                hi[a] = std::max(hi[a], p[a]);
            }
        }

        ProbeGrid candidate = source;
        candidate.frame = frame;

        for (unsigned a = 0; a < 2; ++a) {
            // A full interpolation cell around every required receiver keeps boundary fading
            // away from the supported floor and its walls.
            lo[a] -= candidate.spacing[a];
            hi[a] += candidate.spacing[a];
            candidate.dimensions[a] =
                std::max(2u, unsigned(std::ceil((hi[a] - lo[a]) / candidate.spacing[a])) + 1);
        }

        candidate.origin = frame.to_world(lo);

        if (candidate.probe_count() < best.probe_count()) {
            best = candidate;
        }
    }

    if (refine_corridors) {
        best = refine_cross_section(best, source, receivers, layout.clearance[nearest]);
    }

    best.validate();
    return best;
}
}  // namespace openrac::lighting::bake_detail
