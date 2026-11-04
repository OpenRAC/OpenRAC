// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Two-sided collision coverage expansion used before dense-region partitioning.
#pragma once

#include <array>
#include <map>
#include <vector>

#include "common/result.h"
#include "enhanced/lighting/interiors.h"

namespace openrac::lighting {
// Double precision retains the planner's lattice arithmetic independently of Embree's float rays.
using CoveragePoint = std::array<double, 3>;

// One gameplay seed and the actual source floors allowed to start its enclosure flood.
struct CoverageSeed {
    std::array<double, 6> bounds{};      // Original world-space probe bounds.
    std::vector<CoveragePoint> anchors;  // Connected gameplay floor evidence.
    bool require_anchor = false;         // Authored transition seeds do not require moby evidence.
};

// Shared lattice bounds and enclosure limits, resolved by the existing Python planner.
struct CoverageSettings {
    CoveragePoint lower{};    // Coarse lattice's lower supported coordinate.
    CoveragePoint upper{};    // Coarse lattice's upper supported coordinate.
    CoveragePoint spacing{};  // Dense lattice spacing on each axis.
    double max_rise = 32;     // Maximum extension above an original seed.
    double padding = 3;       // Support beyond confirmed ceilings, at least vertical spacing.
};

// Coverage cells and counters; rectangle merging remains in the region planner.
struct CoverageResult {
    std::map<std::array<int, 2>, std::array<double, 2>> cells;  // XY cell to low/high bounds.
    unsigned indoor = 0;                                        // Accepted enclosed samples.
    unsigned limited = 0;               // Ceilings clipped by the configured maximum rise.
    unsigned discovered = 0;            // Cells added beyond the original seed.
    unsigned disconnected = 0;          // Initial samples without gameplay-floor connectivity.
    unsigned seen = 0;                  // Unique attempted samples, including outdoor failures.
    bool sample_limit_reached = false;  // Flood stopped at its bounded work limit.
};

// Expands all seeds against one collision BVH; malformed settings return an error.
[[nodiscard]] Result<std::vector<CoverageResult>> extend_surface_coverage(
    const std::vector<interiors::Triangle>& triangles,
    const std::vector<CoverageSeed>& seeds,
    const CoverageSettings& settings
);
}  // namespace openrac::lighting
