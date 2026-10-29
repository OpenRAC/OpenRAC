// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Bounded playable space and an unsigned-distance-field medial graph.
#pragma once
#include "enhanced/lighting/environment_bake.h"

namespace openrac::lighting::bake_detail {
struct Scene;
struct RoomDistanceField;

// Traversal type is independent of lighting-space ownership and capture merging.
enum class RoomLinkKind {
    Walk,
    Jump,
    Swim
};

// A gameplay connection; airborne portions never become artificial solid-floor coverage.
struct RoomLink {
    unsigned from = 0, to = 0;  // Indices into medial_points.
    float apex = 0;             // Highest point of the collision-checked trajectory, in world Z.
    RoomLinkKind kind = RoomLinkKind::Jump;
};

// Shared normalised-time trajectory for collision checks and the debug curve.
Float3 room_jump_position(Float3 a, Float3 b, float apex, float t);

// Repair short, continuously supported gaps without treating sampled component IDs as rooms.
std::vector<EnvironmentFloorSample> connected_room_floors(
    const Scene& scene,
    std::span<const EnvironmentFloorSample> floors,
    const BakeSettings& settings,
    EnvironmentPlacementReport* report = nullptr
);

struct RoomLayout {
    std::vector<unsigned> owners;
    std::vector<float> clearance;
    std::vector<Float2>
        height_bounds;  // Analysis columns: existing exterior bounds or enclosed ceilings.
    std::vector<float> ceilings;        // Confirmed enclosed ceilings; infinity outdoors.
    std::vector<Float3> medial_points;  // Subcell ridge positions on supported floors.
    std::vector<unsigned> medial_owners;
    float medial_cell_size = 1;
    unsigned medial_ridge_samples = 0;
    std::vector<std::array<unsigned, 2>> medial_edges;  // Indices into medial_points.
    std::vector<unsigned> medial_centres;  // One seed for each connected floor component.
    std::vector<RoomLink> links;
    void rebuild_medial(
        const Scene& scene,
        std::span<const EnvironmentFloorSample> floors,
        const BakeSettings& settings
    );
    std::vector<std::vector<unsigned>> rooms;
    unsigned centre(std::span<const EnvironmentFloorSample> floors, unsigned room) const;
    // Tangent of the nearest medial segment; open room centres have no preferred direction.
    ProbeFrame frame(
        std::span<const EnvironmentFloorSample> floors,
        unsigned room,
        Float3 position,
        float radius = 24
    ) const;
};

// Add directed airborne edges between walking components without merging their lighting spaces.
void connect_room_links(
    RoomLayout& layout,
    const RoomDistanceField& field,
    const Scene& scene,
    const BakeSettings& settings
);

RoomLayout prepare_room_layout(
    const BakeScene& scene,
    std::vector<EnvironmentFloorSample>& floors,
    const BakeSettings& settings,
    unsigned threads = 0
);

ProbeGrid fit_room_grid(
    const ProbeGrid& source,
    const BakeScene& scene,
    std::span<const EnvironmentFloorSample> floors,
    const RoomLayout& layout,
    bool refine_corridors = false
);

// Floor edges already establish collision walkability. Render geometry supplies wall clearance;
// floor height and authored transitions remain hard cuts, while local GI/sky noise does not.
RoomLayout room_layout(
    const Scene& scene,
    std::span<const EnvironmentFloorSample> floors,
    const BakeSettings& settings,
    bool build_medial = true,
    bool split_doorways = false
);
}  // namespace openrac::lighting::bake_detail
