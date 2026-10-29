// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Offline simplification against full-density directional lighting and visibility.
#pragma once
#include "enhanced/lighting/room_layout.h"

namespace openrac::lighting::bake_detail {
struct ProbeDensitySettings {
    bool enabled = false;
    float relative_error = .1f;
    float absolute_error = .02f;  // Linear irradiance; dark references use a stricter floor.
    void validate() const;
};

struct ProbeDensityReceiver {
    Float3 position{}, normal{};
};

struct ProbeDensityReport {
    std::size_t original_probes = 0, final_probes = 0, receivers = 0, comparisons = 0;
    unsigned accepted_axes = 0, rejected_axes = 0;
    float maximum_error = 0;
};

// Camera-facing and four oblique directions used by both local and whole-field verification.
std::array<Float3, 5> probe_density_views(Float3 normal);
bool probe_density_matches(
    Float3 reference, Float3 candidate, const ProbeDensitySettings& settings, float& maximum_error
);

// Player and jump heights, plus two-sided wall/ceiling hits in the reachable floor domain.
std::vector<ProbeDensityReceiver> probe_density_receivers(
    const BakeScene& scene,
    std::span<const EnvironmentFloorSample> floors,
    const RoomLayout& layout,
    const BakeSettings& settings,
    unsigned threads = 0
);

// Keep exact original capture positions, moments and endpoints. Each axis may double its spacing;
// rejected candidates retain the fine grid. No candidate is compared against an earlier reduction.
ProbeVolume simplify_probe_density(
    ProbeVolume reference,
    const ProbeVolume& coarse,
    std::span<const ProbeDensityReceiver> receivers,
    const ProbeDensitySettings& settings,
    ProbeDensityReport& report
);
}  // namespace openrac::lighting::bake_detail
