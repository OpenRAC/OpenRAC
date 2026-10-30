// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once
#include <map>
#include <tuple>

#include "enhanced/lighting/environment_bake.h"

namespace openrac::lighting::bake_detail {
struct Scene;
struct RoomLayout;

struct RoomWallDistance {
    float distance = 32;
    Float3 closest{};
    bool found = false;
};

// Unsigned Euclidean distance to the geometry's horizontal cross-section. No authored normals,
// floor planes or artificial analysis caps participate in the horizontal field.
RoomWallDistance room_wall_distance(const Scene& scene, Float3 position);

struct RoomFieldNode {
    Float3 position{};  // Supporting floor, not the sampling height.
    RoomWallDistance wall;
    unsigned room = 0;
    bool ridge = false;
    std::vector<unsigned> neighbours;
};

struct RoomDistanceField {
    float step = 1;
    std::vector<RoomFieldNode> nodes;
    std::map<std::tuple<unsigned, int, int>, std::vector<float>> domain;
    std::vector<bool> swimming_rooms;
    std::vector<bool> submerged_rooms;
    std::vector<std::optional<float>> water_heights;
    bool contains(Float3 position, unsigned room) const;
    bool connects(Float3 a, Float3 b, unsigned room) const;
};

RoomDistanceField room_distance_field(
    const Scene& scene,
    std::span<const EnvironmentFloorSample> floors,
    const RoomLayout& layout,
    const BakeSettings& settings
);
std::optional<Float3> room_floor_position(
    const Scene& scene, Float3 position, const BakeSettings& settings
);
// Recheck real floor support and body/head visibility before smoothing or adding a shortcut.
bool room_path_clear(const Scene& scene, Float3 a, Float3 b, const BakeSettings& settings);
// Body/head clearance without requiring support between endpoints, for swimming and jump approaches.
bool room_body_path_clear(
    const Scene& scene, Float3 a, Float3 b, const BakeSettings& settings, bool swimming = false
);
// Follow supported floor through a bounded height dip instead of requiring a flat endpoint chord.
// Empty means the route has a missing floor, excessive step, wall or insufficient player clearance.
std::vector<Float3> room_floor_path(
    const Scene& scene, Float3 a, Float3 b, const BakeSettings& settings
);
}  // namespace openrac::lighting::bake_detail
