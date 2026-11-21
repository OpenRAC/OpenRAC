// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once
#include "enhanced/lighting/bake_scene.h"
#include "enhanced/lighting/environment_bake.h"

namespace openrac::lighting::bake_detail {
// Supported visible floor paired with its capture height, enclosure and completed-GI signature.
struct EnvironmentFloor {
    Float3 position{}, capture{}, light{};
    ProbeFrame frame;
    unsigned source = 0, room = ~0u, sky_band = 0;
    std::optional<std::vector<unsigned>>
        lighting_labels{};  // Authored mood identity, separate from floor fragments.
    float clearance = 0;
    float locality_radius = 0;      // Measured open-sky reach; zero retains the nominal spacing.
    float area = 1;                 // Approximate horizontal footprint in world units squared.
    bool capture_candidate = true;  // Low passages still receive reflections from a nearby capture.
    std::optional<float> water_height{};
    bool underwater = false;
    bool swimming_surface = false;
    std::optional<unsigned> island{};
    unsigned component = ~0u;
    float receiver_height = 2.5f;  // Submerged samples remain below the authored waterline.
    std::vector<unsigned>
        neighbours;  // Supported floor graph; proximity across a wall is not an edge.
};

struct EnvironmentFloorMap {
    std::vector<EnvironmentFloor> floors;
    std::vector<std::vector<unsigned>> rooms;
};

// Broad brightness/hue compatibility permits blending between similarly lit connected spaces.
bool similar_environment_light(Float3 a, Float3 b);
// Join compatible sampled components over short, continuously supported paths before room
// segmentation. Geometry still splits narrow doorways, walls and disconnected storeys.
void connect_environment_floor_fragments(
    const Scene& scene,
    std::vector<EnvironmentFloorSample>& samples,
    std::span<const EnvironmentFloor> floors,
    const BakeSettings& settings,
    const EnvironmentBakeSettings& options,
    EnvironmentPlacementReport& report,
    bool preserve_doorways = true
);
// Merge compatible fragments across direct edges or short gaps left by rejected capture locations.
void merge_environment_spaces(
    const Scene& scene,
    const BakeSettings& settings,
    std::vector<EnvironmentFloor>& floors,
    std::span<const EnvironmentFloorSample> samples,
    std::vector<std::vector<unsigned>>& rooms,
    EnvironmentPlacementReport& report,
    float minimum_space_area = 0,
    const EnvironmentBakeSettings& options = {},
    bool preserve_islands = false
);
// Reachability uses physical edges; lighting segmentation preserves authored transition cuts.
EnvironmentFloorMap environment_floor_map(
    const Scene& scene,
    std::span<const ProbeVolume> volumes,
    std::span<const EnvironmentFloorSample> samples,
    const BakeSettings& settings,
    const EnvironmentBakeSettings& options,
    EnvironmentPlacementReport& report,
    const Scene* gameplay_collision = nullptr
);
}  // namespace openrac::lighting::bake_detail
