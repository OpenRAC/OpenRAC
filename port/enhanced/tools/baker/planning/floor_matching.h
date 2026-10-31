// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Spatially indexed floor evidence shared by GI fitting and reflection placement.
#pragma once
#include <span>

#include "common/json.h"
#include "common/result.h"
#include "enhanced/lighting/environment_bake.h"

namespace openrac::lighting {
// Marks gameplay anchors and height bounds without changing floor order or nearest-floor ties.
// Floors must have finite coordinates; invalid or missing anchor lists return an error.
[[nodiscard]] Result<void> match_floor_evidence(
    std::span<EnvironmentFloorSample> floors, const json::Value& seeds
);
}  // namespace openrac::lighting
