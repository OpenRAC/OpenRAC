// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once
#include "enhanced/lighting/environment_floor_map.h"
#include "enhanced/lighting/environment_neighbourhood.h"

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
// Capture sharing uses actual visibility and graph locality, including ramps.
bool environment_capture_visible(
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
// Fit assigned local receivers without losing blend strength or leaking outside
// the neighbourhood. Return captures whose domains require another local seed.
std::vector<unsigned> fit_environment_capture_boxes(
    const Scene& scene,
    std::span<const EnvironmentFloor> floors,
    const BakeSettings& settings,
    const EnvironmentBakeSettings& options,
    std::vector<EnvironmentProbe>& probes,
    std::span<const unsigned> origins,
    const EnvironmentAssignments* assignments = nullptr,
    EnvironmentNeighbourhoods* neighbourhoods = nullptr,
    std::span<const float> inherited_margins = {}
);
// Trial-remove a capture, transferring its unique receivers to compatible local
// neighbours and refitting their boxes. Commit only when every transfer fits.
bool remove_environment_capture(
    const Scene& scene,
    std::span<const EnvironmentFloor> floors,
    const BakeSettings& settings,
    const EnvironmentBakeSettings& options,
    unsigned removed,
    std::vector<EnvironmentProbe>& probes,
    std::vector<unsigned>& origins,
    EnvironmentAssignments& assignments,
    EnvironmentNeighbourhoods& neighbourhoods
);
// Preserve mandatory coverage, propose independent local cameras, and select
// photographs by coverage and enclosure-normalised receiver error.
std::vector<EnvironmentProbe> select_environment_captures(
    const Scene& scene,
    const BakeSettings& settings,
    const EnvironmentBakeSettings& options,
    std::span<const EnvironmentFloor> floors,
    std::span<const EnvironmentProbe> regions,
    std::span<const unsigned> origins,
    EnvironmentNeighbourhoods& neighbourhoods,
    EnvironmentPlacementReport& report,
    std::vector<EnvironmentInfluence>& influences
);
}  // namespace openrac::lighting::bake_detail
