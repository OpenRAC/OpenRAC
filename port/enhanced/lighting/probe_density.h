// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Offline simplification against full-density directional lighting and visibility.
#pragma once
#include <functional>

#include "enhanced/lighting/bake_workers.h"
#include "enhanced/lighting/room_layout.h"

namespace openrac::lighting::bake_detail {
struct ProbeDensitySettings {
    bool enabled = false;
    float relative_error = .1f;
    float absolute_error = .02f;  // Linear irradiance; dark references use a stricter floor.
    ProbeTextureFormat texture_format =
        ProbeTextureFormat::R11G11B10;  // Compare the actual stored colours.
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

// Borrows unchanged comparison evidence and the controller's execution budget until reduction returns.
struct ProbeDensityInput {
    const ProbeVolume& coarse;  // Background field used by the final sparse/local blend.
    std::span<const ProbeDensityReceiver>
        receivers;               // Playable surfaces that must retain lighting.
    BakeExecution execution{1};  // Compression shares workers without changing density settings.
    // Optional identity function finalizes candidate headers before their first and only encoding.
    std::function<std::uint64_t(const ProbeGrid&)> settings_hash{};
};

// Owns the chosen raw grid and its already encoded payload, plus a fallback only after reduction.
struct ProbeDensityReduction {
    ProbeVolume volume;  // Chosen lattice, retaining original unquantized samples and metadata.
    std::vector<std::uint8_t> bytes;  // Chosen GPU-format asset, never re-encoded by output.
    std::vector<std::uint8_t> reference_bytes;  // Original asset, present only for reduced grids.
    ProbeDensityReport
        report;  // The same receiver checks and reduction decisions as serial baking.
};

// Keep exact original capture positions, moments and endpoints. Each axis may double its spacing;
// rejected candidates retain the fine grid. No candidate is compared against an earlier reduction.
ProbeDensityReduction simplify_probe_density(
    ProbeVolume reference, const ProbeDensityInput& input, const ProbeDensitySettings& settings
);
}  // namespace openrac::lighting::bake_detail
