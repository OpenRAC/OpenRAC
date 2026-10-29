// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Optional per-level reflection bake following successful GI read-back verification.
#pragma once
#include <filesystem>
#include <ostream>

#include "enhanced/lighting/environment_bake.h"

namespace openrac::lighting {
// Collision-supported gameplay floor graph shared by GI and reflection placement.
std::vector<EnvironmentFloorSample> read_environment_floors(
    const std::filesystem::path& output,
    const std::filesystem::path& level = {},
    const BakeTraversalSettings& traversal = {}
);
// Reuse prepared collision input; no dependency on the water renderer or its optional bake.
void append_exported_water(
    const std::filesystem::path& output,
    std::vector<EnvironmentFloorSample>& floors,
    const std::filesystem::path& level = {},
    const BakeTraversalSettings& traversal = {}
);

// Reads environment_probes from the exported probe-bake.json; disabled when absent.
void bake_level_environment(
    const std::filesystem::path& level,
    const std::filesystem::path& output,
    const BakeScene& scene,
    const BakeSettings& settings,
    unsigned threads,
    std::ostream& log
);
}  // namespace openrac::lighting
