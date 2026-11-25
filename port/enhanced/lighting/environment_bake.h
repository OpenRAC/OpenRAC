// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Static reflection capture after the final diffuse GI assets have been verified.
#pragma once
#include "enhanced/lighting/bake.h"
#include "enhanced/lighting/bake_workers.h"
#include "enhanced/lighting/environment.h"

namespace openrac::lighting {
namespace bake_detail {
struct SpatialPlacement;
}

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
    bool swimming_surface = false;     // A virtual gameplay surface, not solid support geometry.
    bool underwater = false;           // Reflection captures never merge across the waterline.
    std::optional<unsigned> island{};  // Shared pre-GI watershed ownership; refinements inherit it.
    float island_clearance = 0;
    std::optional<float> water_bottom{};  // Collision-confirmed basin depth for swimmer clearance.
    unsigned evidence = 0;  // Debug: 1 gameplay anchor, 2 authored playable area, 3 swimming entry.
    bool gameplay_anchor = false;  // Retained when a manual area overlaps gameplay evidence.
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
    float minimum_swim_depth = 1,
    const BakeTraversalSettings& traversal = {}
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
    bool layered_regions = false;

    struct SurfaceSample {
        Float3 position{}, normal{};
        float area = 0, coverage = 0;
        unsigned owner = ~0u, overlaps = 0;
        unsigned capture_overlaps = 0;
    };

    struct SurfaceCoverage {
        std::vector<SurfaceSample> samples;
        std::vector<unsigned> parents;
        double relevant_area = 0, covered_area = 0, weak_area = 0, uncovered_area = 0;
        double overlap_area = 0;
        double initial_covered_area = 0, initial_overlap_area = 0;
        double weighted_overlaps = 0, initial_weighted_overlaps = 0;
        double distinct_overlaps = 0, initial_distinct_overlaps = 0;
        unsigned added_regions = 0;
        bool budget_limited = false;
    } surface_coverage;

    struct WatershedSample {
        Float3 position;
        unsigned basin;
        float clearance;
        bool water_surface = false;
    };

    std::vector<WatershedSample> watershed;
    std::vector<float> floor_locality_radii;
    std::vector<bool> floor_roofs;
    unsigned input_floors = 0, playable_floors = 0, rejected_floors = 0;
    unsigned unsupported_floors = 0, lighting_spaces = 0, validation_samples = 0;
    unsigned receiver_only_floors =
        0;  // Supported floors with insufficient clearance for a capture.
    unsigned ownership_conflicts = 0, unpruned_probes = 0, pruned_probes = 0;

    struct PlacementConflict {
        Float3 position, origin;
        unsigned capture, source_floor, receiver_floor;
        float distance, limit;
        bool ownership;
    };

    std::vector<PlacementConflict> placement_conflicts;
    unsigned locality_conflicts = 0;
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
    // Bounds are indexed by influence region; positions/owners by photograph.
    std::vector<unsigned> influence_captures, influence_owners;
    std::vector<EnvironmentProbe> capture_projections;  // One parallax proxy per influence region.
    std::vector<Float3> uncovered, unsupported;
    std::vector<unsigned> uncovered_owners;
    std::vector<EnvironmentSpaceDiagnostic> spaces;
    std::uint64_t coverage_hash = 0;
    std::vector<EnvironmentCoverageReceiver> receivers;
};

// Slightly above the 2.5-unit player clearance used by walkability inference.
inline constexpr float kEnvironmentCaptureHeight = 3;

// Expand finalized influence only; photographs, frames, proxies and allocation
// are unchanged. Indoor columns reach collision ceilings; open sky gets 3x the
// ten-unit analysis height. Submerged influence stays below the waterline.
void extend_environment_influence_coverage(
    const BakeScene& collision,
    std::span<const EnvironmentFloorSample> floors,
    const BakeSettings& settings,
    std::span<EnvironmentInfluence> influences
);

// Read-only surface coverage audit. Reports gaps and overlaps without changing
// captures, influence regions, allocation or projection geometry.
EnvironmentPlacementReport::SurfaceCoverage measure_environment_surface_coverage(
    const BakeScene& render,
    const BakeScene& collision,
    std::span<const EnvironmentFloorSample> floors,
    const BakeSettings& settings,
    std::span<const EnvironmentInfluence> influences,
    std::span<const EnvironmentProbe> captures = {},
    const BakeExecution& execution = {}
);

// Coverage-driven captures above reachable gameplay floors, after the final GI field is available.
EnvironmentAsset bake_environment(
    const BakeScene& scene,
    const BakeSettings& settings,
    std::span<const ProbeVolume> gi,
    std::span<const EnvironmentFloorSample> floors,
    const EnvironmentBakeSettings& options = {},
    const BakeExecution& execution = {},
    EnvironmentPlacementReport* placement = nullptr,
    const BakeScene* gameplay_collision = nullptr,
    const bake_detail::SpatialPlacement* spatial = nullptr
);
}  // namespace openrac::lighting
