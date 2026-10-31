// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Static reflection capture after the final diffuse GI assets have been verified.
#pragma once
#include "enhanced/lighting/bake.h"
#include "enhanced/lighting/bake_workers.h"
#include "enhanced/lighting/environment.h"

namespace openrac::lighting {
// Collision floor graph, authored lighting component, sky exposure and gameplay anchors.
struct EnvironmentFloorSample {
    Float3 position{};
    unsigned region = 0;
    float sky = 0;
    bool anchor = false;
    std::vector<unsigned> neighbours;
    std::optional<Float2> height_bounds{};  // Existing coverage envelope, in world Z.
    // Component IDs describe floor connectivity, not authored moods. Null is legacy/unknown
    // evidence; an empty list explicitly means no authored label was assigned.
    std::optional<std::vector<unsigned>> lighting_labels{};
    // Authored swimmable collision surface above this location.
    std::optional<float> water_height{};
    bool swimming_surface = false;  // A virtual gameplay surface, not solid support geometry.
    bool underwater = false;        // Reflection captures never merge across the waterline.
};

// Collision-derived gameplay geometry; swimmable surfaces are not solid walking support.
struct GameplayTriangle {
    std::array<Float3, 3> vertices{};
    bool swimmable = false;
};

// Extend reachable land samples over authored swimmable surfaces, with separate submerged receivers.
void append_swimming_surfaces(
    std::span<const GameplayTriangle> triangles,
    std::vector<EnvironmentFloorSample>& floors,
    float spacing = 2,
    const std::function<bool(Float3)>& wet_mask = {},
    float minimum_swim_depth = 1
);

struct EnvironmentSpaceDiagnostic {
    unsigned cells = 0, authored_region = 0, sky_band = 0;
    Float3 minimum{}, maximum{}, light{};
    std::vector<unsigned> neighbours;
    bool underwater = false;
    std::optional<float> water_height{};
};

struct EnvironmentProbeMerge {
    Float3 a{}, b{}, replacement{};
    float overlap = 0, loss_fraction = 0;
    unsigned trials = 0;
};

struct EnvironmentCaptureMove {
    Float3 from{}, to{};
};

struct EnvironmentPlacementReport {
    unsigned input_floors = 0, playable_floors = 0, rejected_floors = 0;
    unsigned unsupported_floors = 0, lighting_spaces = 0, validation_samples = 0;
    unsigned receiver_only_floors =
        0;  // Supported floors with insufficient clearance for a capture.
    unsigned ownership_conflicts = 0, unpruned_probes = 0, pruned_probes = 0;
    unsigned spaces_before_merge = 0, merged_fragments = 0, merged_graph_gaps = 0;
    unsigned merged_floor_components = 0, bridged_floor_edges = 0;
    float minimum_coverage = 1, covered_fraction = 1;
    unsigned merge_pair_attempts = 0, merge_position_trials = 0;
    std::vector<EnvironmentProbeMerge> probe_merges;
    std::vector<Float3> capture_positions;
    std::vector<EnvironmentProbe>
        capture_bounds;  // Retain failed placement bounds for coverage diagnosis.
    std::vector<unsigned>
        submerged_captures;  // Stable capture IDs for inspecting waterline separation.
    std::vector<EnvironmentCaptureMove> capture_moves;
    unsigned occlusion_separated_pairs = 0;
    std::vector<std::array<unsigned, 2>> spacing_conflicts;
    std::vector<unsigned> tolerated_receivers;
    std::vector<Float3> tolerated_misses;
    std::vector<unsigned> owners;
    std::vector<Float3> uncovered, unsupported;
    std::vector<unsigned> uncovered_owners;
    std::vector<EnvironmentSpaceDiagnostic> spaces;
    std::uint64_t coverage_hash = 0;
    std::vector<EnvironmentCoverageReceiver> receivers;
};

// Slightly above the 2.5-unit player clearance used by walkability inference.
inline constexpr float kEnvironmentCaptureHeight = 3;

// Coverage-driven captures above reachable gameplay floors, after the final GI field is available.
EnvironmentAsset bake_environment(
    const BakeScene& scene,
    const BakeSettings& settings,
    std::span<const ProbeVolume> gi,
    std::span<const EnvironmentFloorSample> floors,
    const EnvironmentBakeSettings& options = {},
    const BakeExecution& execution = {},
    EnvironmentPlacementReport* placement = nullptr
);
}  // namespace openrac::lighting
