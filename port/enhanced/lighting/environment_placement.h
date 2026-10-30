// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once
#include "enhanced/lighting/bake_scene.h"
#include "enhanced/lighting/environment_bake.h"

namespace openrac::lighting::bake_detail {
// Ground-supported centres; nearby captures across walls remain independent.
std::vector<EnvironmentProbe> place_environment_probes(
    const Scene& scene,
    std::span<const ProbeVolume> volumes,
    std::span<const EnvironmentFloorSample> floors,
    const BakeSettings& settings,
    const EnvironmentBakeSettings& options,
    EnvironmentPlacementReport* report = nullptr
);
}  // namespace openrac::lighting::bake_detail
