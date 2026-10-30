// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Optional per-level reflection bake following successful GI read-back verification.
#pragma once
#include <filesystem>
#include <ostream>
#include <span>

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

// Borrowed inputs shared with the completed GI stage; the caller owns them until reflections drain.
struct EnvironmentStageInput {
    const std::filesystem::path& level;   // Exported level directory.
    const std::filesystem::path& output;  // Verified GI directory and reflection output directory.
    const BakeScene& scene;               // Immutable transport geometry and lighting.
    const BakeSettings& settings;         // Shared transport and traversal settings.
    std::span<const EnvironmentFloorSample> floors;  // Prepared playable floors, without copying.
    std::span<const ProbeVolume>
        verified_field{};  // Optional completed density field, borrowed until return.
};

// Reads environment_probes from the exported probe-bake.json; disabled when absent.
// Empty prepared floors retain standalone loading and its existing missing-input diagnostics.
void bake_level_environment(
    const EnvironmentStageInput& input, const BakeExecution& execution, std::ostream& log
);
}  // namespace openrac::lighting
