// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Small resident line meshes show the complete reflection-probe coverage without changing selection.
#include "enhanced/viewer/environment_overlay.h"

#include <cmath>
#include <numbers>

namespace openrac::lighting {
namespace {
constexpr unsigned kCircleSegments = 24;
constexpr float kMarkerRadius = .75f;
constexpr float kCrossRadius = .35f;
constexpr Float3 kPositionColour{1, 1, 1};
constexpr Float3 kBoundsColour{.1f, .8f, 1};

// Three great circles remain recognisable as a sphere from every camera angle.
void append_marker(
    std::vector<DebugLineVertex>& vertices, Float3 position, Float3 colour = kPositionColour
) {
    for (unsigned axis = 0; axis < 3; ++axis) {
        for (unsigned segment = 0; segment < kCircleSegments; ++segment) {
            for (unsigned endpoint = 0; endpoint < 2; ++endpoint) {
                const float angle =
                    2 * std::numbers::pi_v<float> * float(segment + endpoint) / kCircleSegments;
                auto p = position;
                p[(axis + 1) % 3] += kMarkerRadius * std::cos(angle);
                p[(axis + 2) % 3] += kMarkerRadius * std::sin(angle);
                vertices.push_back({p, colour});
            }
        }
        // A centred cross distinguishes the exact capture point from the surrounding marker.
        for (float sign : {-1.f, 1.f}) {
            auto p = position;
            p[axis] += sign * kCrossRadius;
            vertices.push_back({p, colour});
        }
    }
}
}  // namespace

std::vector<DebugLineVertex> environment_probe_debug_vertices(
    std::span<const EnvironmentProbe> probes
) {
    constexpr unsigned kVerticesPerProbe = 24 + 3 * kCircleSegments * 2 + 6;
    std::vector<DebugLineVertex> vertices;
    vertices.reserve(probes.size() * kVerticesPerProbe);
    for (const auto& probe : probes) {
        const auto first = vertices.size();
        append_debug_box(vertices, probe.minimum, probe.maximum, kBoundsColour);
        for (std::size_t n = first; n < vertices.size(); ++n) {
            vertices[n].position = probe.frame.to_world(vertices[n].position);
        }
        append_marker(vertices, probe.position);
    }
    return vertices;
}

}  // namespace openrac::lighting
