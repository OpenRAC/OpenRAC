// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once
#include <functional>

#include "enhanced/lighting/room_layout.h"
#include "enhanced/lighting/spatial_volume_fit.h"

namespace openrac::lighting::bake_detail {
struct SpatialPlacement {
    SpatialWatershed field;
    SpatialVolumePlan gi, reflection;
    float air_height = 0;
    bool gi_rejected = false;
};

SpatialPlacement build_spatial_placement(
    const BakeScene& geometry,
    std::span<const EnvironmentFloorSample> floors,
    const BakeSettings& settings,
    Float3 spacing,
    unsigned maximum_probes,
    unsigned maximum_grids,
    const EnvironmentBakeSettings& environment,
    const BakeExecution& execution = {1},
    const std::function<void(const SpatialPlacement&)>& on_gi_failure = {}
);
RoomLayout spatial_floor_layout(
    const SpatialWatershed& field, std::vector<EnvironmentFloorSample>& floors
);
std::vector<EnvironmentProbe> place_spatial_environment_probes(
    const BakeScene& render,
    const BakeScene& collision,
    std::span<const ProbeVolume> gi,
    std::span<const EnvironmentFloorSample> floors,
    const BakeSettings& settings,
    const EnvironmentBakeSettings& options,
    const SpatialPlacement& spatial,
    EnvironmentPlacementReport& report,
    std::vector<EnvironmentInfluence>& influences
);
}  // namespace openrac::lighting::bake_detail
