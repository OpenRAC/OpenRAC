// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Probe allocations for whole levels, world-space bounds and exact lattice endpoints.

#pragma once

#include <array>

#include "enhanced/lighting/probes.h"

namespace openrac::lighting {

// Scene geometry and receiver bounds, owned by the caller.
struct BakeSceneExport;

// Visibility moments cover 1.5 cell diagonals; transport and occupancy trace
// the full scene. Scene bounds determine allocation, not the visibility range.
ProbeGrid sparse_probe_grid(const BakeSceneExport& scene, float spacing, unsigned resolution);
ProbeGrid sparse_probe_grid(const BakeSceneExport& scene, Float3 spacing, unsigned resolution);
// Cell-aligned local bounds; transport still uses the entire exported scene.
ProbeGrid bounded_probe_grid(
    const BakeSceneExport& scene, Float3 minimum, Float3 maximum, float spacing, unsigned resolution
);
ProbeGrid bounded_probe_grid(
    const BakeSceneExport& scene,
    Float3 minimum,
    Float3 maximum,
    Float3 spacing,
    unsigned resolution
);
// Builds exact lattice endpoints without repeating floating bounds division or ceil operations.
// Rejects noninteger cells, reversed bounds and dimensions outside the supported grid range.
[[nodiscard]] ProbeGrid lattice_probe_grid(
    const BakeSceneExport& scene,
    const std::array<float, 6>& cells,
    Float3 spacing,
    unsigned resolution
);
}  // namespace openrac::lighting
