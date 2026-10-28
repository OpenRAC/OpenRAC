// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Player-containing nominal probe boxes, independent of the camera's lighting selection.

#pragma once

#include <span>
#include <vector>

#include "enhanced/lighting/probes.h"
#include "enhanced/viewer/debug_lines.h"

namespace openrac::lighting {

// Returns every containing dense index, or -1 for the containing sparse fallback; outside returns none.
std::vector<int> containing_probe_volumes(
    const ProbeGrid& sparse, std::span<const ProbeGrid> dense, Float3 player
);

// Builds all twelve nominal box edges: yellow for dense regions, cyan for the sparse fallback.
std::vector<DebugLineVertex> probe_volume_debug_vertices(
    const ProbeGrid& sparse, std::span<const ProbeGrid> dense, std::span<const int> indices
);

}  // namespace openrac::lighting
