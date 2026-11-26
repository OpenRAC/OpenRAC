// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// One physically connected floor domain shared by GI orientation and reflection placement.
#include "enhanced/lighting/environment_floor_map.h"
#include "enhanced/lighting/room_layout.h"

namespace openrac::lighting::bake_detail {
std::vector<EnvironmentFloorSample> connected_room_floors(
    const Scene& scene,
    std::span<const EnvironmentFloorSample> source,
    const BakeSettings& settings,
    EnvironmentPlacementReport* report
) {
    std::vector<EnvironmentFloorSample> result(source.begin(), source.end());
    std::vector<EnvironmentFloor> supported;

    for (const auto& sample : source) {
        EnvironmentFloor floor;
        floor.position = sample.position;
        floor.capture = sample.position;
        floor.capture[2] += kEnvironmentCaptureHeight;
        // Authored labels constrain this physical phase; actual GI refines reflection rooms later.
        floor.light = {1, 1, 1};
        supported.push_back(floor);
    }

    EnvironmentPlacementReport local;
    connect_environment_floor_fragments(
        scene, result, supported, settings, {}, report ? *report : local, false
    );
    // Path raycasts validate connectivity only; they must not become floor coverage samples.
    return result;
}
}  // namespace openrac::lighting::bake_detail
