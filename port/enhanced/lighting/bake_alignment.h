// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Offline probe alignment within fixed coverage bounds, before individual relocation.

#pragma once

#include "enhanced/lighting/bake.h"
#include "enhanced/lighting/bake_workers.h"

namespace openrac::lighting::bake_detail {
struct Scene;

// Mean capped-clearance score and placement counts for one candidate grid phase.
struct AlignmentScore {
    double mean = 0;         // Normalised squared clearance minus near/inside penalties.
    std::size_t near = 0;    // Probes below the desired surface clearance.
    std::size_t inside = 0;  // Probes classified inside geometry by paired crossing parity.
};

// Chosen shared interior offset; outer-face coordinates remain anchored for coverage.
struct GridAlignment {
    Float3 offset{};        // Applied before cell-local relocation; zero leaves the grid unchanged.
    AlignmentScore before;  // Full-grid score without alignment.
    AlignmentScore after;   // Full-grid score of the accepted placement.
};

// Aligns initial probe positions deterministically, retaining dimensions, bounds and the offset budget.
// The volume must be freshly allocated, with zero offsets; no other task may modify it.
// Shared workers evaluate rays; the caller reduces scores and resolves ties in the original order.
// Without a pool, evaluation stays on the caller; ProbeBaker supplies its shared or local stage pool.
// Scores remain unmeasured at zero when the preview finds no candidate worth a full-grid check.
GridAlignment align_probe_grid(
    const Scene& scene,
    const BakeSettings& settings,
    ProbeVolume& volume,
    const BakeExecution& execution = {}
);

}  // namespace openrac::lighting::bake_detail
