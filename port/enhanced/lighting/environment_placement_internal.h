// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once
#include "enhanced/lighting/environment_floor_map.h"

namespace openrac::lighting::bake_detail {
inline constexpr std::array<float, 3> kEnvironmentReceiverHeights{.15f, 1.25f, 2.5f};
inline constexpr float kEnvironmentUsefulCoverage = .2f;

// Preserve the three receiver levels while fitting submerged levels beneath their waterline.
inline Float3 environment_receiver_position(const EnvironmentFloor& floor, unsigned height) {
    auto p = floor.position;
    p[2] += kEnvironmentReceiverHeights[height] * (floor.receiver_height / 2.5f);
    return p;
}

// Two-sided scene visibility and lighting-space compatibility shared by placement and replacement.
bool environment_segment_clear(
    const Scene& scene, Float3 a, Float3 b, const BakeSettings& settings
);
// A short Euclidean distance across geometry is not the same playable visibility pocket.
bool environment_capture_context_clear(
    const Scene& scene,
    const EnvironmentFloor& a,
    Float3 capture_a,
    const EnvironmentFloor& b,
    Float3 capture_b,
    const BakeSettings& settings
);
bool environment_spaces_compatible(
    const EnvironmentFloor& source, const EnvironmentFloor& receiver
);
// Influence follows the connected lighting space, not visibility from a cubemap's capture point.
bool environment_receiver_supported(
    const EnvironmentProbe& probe,
    const EnvironmentFloor& source,
    const EnvironmentFloor& receiver,
    Float3 position
);
// Derives independent influence and parallax bounds at a supported floor position.
EnvironmentProbe environment_capture_box(
    const Scene& scene,
    const EnvironmentFloor& floor,
    std::span<const unsigned> room,
    std::span<const EnvironmentFloor> floors,
    const BakeSettings& settings,
    const EnvironmentBakeSettings& options
);
}  // namespace openrac::lighting::bake_detail
