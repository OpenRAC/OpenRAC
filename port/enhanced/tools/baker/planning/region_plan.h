// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Automatic native region planning before grid alignment, relocation and lighting transport.
#pragma once
#include <filesystem>
#include <string>

#include "common/json.h"
#include "common/result.h"

namespace openrac::lighting {
// Selects complete local coverage or just the gameplay evidence required by floor analysis.
enum class RegionPlanPurpose {
    Coverage,
    FloorAnchors
};

// Plans complete local coverage from retained gameplay, reviewed profiles and collision geometry.
// The request owns resolved profile/options/coarse inspection and source paths; no geometry is copied.
// Returns the existing seed-plan report as JSON, or a source/settings/coverage error.
[[nodiscard]] Result<std::string> plan_probe_regions(
    const std::filesystem::path& level,
    const json::Value& request,
    RegionPlanPurpose purpose = RegionPlanPurpose::Coverage
);
}  // namespace openrac::lighting
