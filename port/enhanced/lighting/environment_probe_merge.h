// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Reflection capture merge policy and its borrowed placement inputs.

#pragma once
#include "enhanced/lighting/environment_placement_internal.h"

namespace openrac::lighting::bake_detail {
// Borrowed placement inputs, valid for the duration of one serial merge pass.
struct EnvironmentMergeInputs {
    const Scene& scene;                  // Prepared two-sided transport geometry.
    std::span<EnvironmentFloor> floors;  // Caller-owned floor samples and mutable space labels.
    std::vector<std::vector<unsigned>>& rooms;  // Caller-owned floor IDs grouped by space.
    const BakeSettings& settings;               // Transport bias and visibility settings.
    const EnvironmentBakeSettings& options;     // Reflection placement and merge policy.
};

// Borrowed receiver geometry and area budgets for one coverage decision.
struct EnvironmentMergeCoverage {
    std::span<const EnvironmentCoverageReceiver> receivers;  // Player-height receiver samples.
    std::span<const EnvironmentFloor> floors;                // Receiver footprint weights.
    float target_area;                       // Pair coverage area in world units squared.
    float total_area;                        // Whole-level coverage area in world units squared.
    const EnvironmentBakeSettings& options;  // Per-pair coverage limits.
    bool spacing_merge = false;              // True for a mandatory minimum-spacing repair.
};

// Exposure-aware perceptual comparison of the GI signature at two capture locations.
bool similar_environment_capture_light(Float3 a, Float3 b, const EnvironmentBakeSettings& options);
// Small isolated floor misses are allowed only within per-pair and whole-level area budgets.
bool environment_merge_loss_allowed(
    std::span<const unsigned> misses,
    std::span<const unsigned> newly_lost,
    const EnvironmentMergeCoverage& coverage
);
// Merges overlapping captures; close visible fragments share a space before replacement is tested.
void merge_environment_probes(
    const EnvironmentMergeInputs& inputs,
    std::vector<EnvironmentProbe>& probes,
    std::vector<unsigned>& origins,
    EnvironmentPlacementReport& report
);
}  // namespace openrac::lighting::bake_detail
