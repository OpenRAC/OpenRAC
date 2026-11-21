// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once
#include "enhanced/lighting/environment_bake.h"

namespace openrac::lighting::bake_detail {
// Attach generated water receivers to the physical floor graph, then discard unanchored space.
void connect_reachable_water(
    std::span<const GameplayTriangle> triangles,
    std::vector<EnvironmentFloorSample>& floors,
    unsigned original_count,
    float spacing,
    const BakeTraversalSettings& traversal
);
}  // namespace openrac::lighting::bake_detail
