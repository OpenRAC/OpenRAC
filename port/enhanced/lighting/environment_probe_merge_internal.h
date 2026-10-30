// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Reflection capture replacement state, shared only by the offline placement implementation.

#pragma once

#include <set>
#include <tuple>

#include "enhanced/lighting/environment_probe_merge.h"

namespace openrac::lighting::bake_detail {

// Search and whole-level coverage budgets.
constexpr unsigned kCandidateLimit = 32, kGlobalMissLimit = 16;
constexpr float kGlobalLossFraction = .001f;

// A candidate merge and its receiver union.
struct Pair {
    unsigned a = 0, b = 0;
    float overlap = 0;
    std::vector<unsigned> targets;
    bool close = false;
    float distance_squared = 0;
};

// A tested replacement capture and its retained receiver set.
struct Replacement {
    unsigned floor = ~0u, trials = 0;
    EnvironmentProbe probe;
    std::vector<unsigned> supported;
    float missed_area = 0, new_loss = 0, quality = 0;
};

// Inputs and best result for one deterministic replacement search; the pair outlives this search.
struct ReplacementSearch {
    const Pair& pair;                    // Candidate captures and required receiver union.
    std::vector<unsigned> contribution;  // Coverage removed with the two old captures.
    Float3 middle;                       // Original capture midpoint used for tie-breaking.
    float target_area;                   // Represented receiver area in world units squared.
    Replacement best;  // Best accepted replacement and number of attempted positions.
};

// A tentative lighting-space union, rolled back when coverage cannot be retained.
struct RoomJoin {
    unsigned destination = 0, source = 0;
    std::size_t previous_size = 0;
    std::vector<unsigned> moved;
};

// Returns one receiver's represented floor area.
float area(unsigned receiver, std::span<const EnvironmentFloor> floors);
// Sums represented area in deterministic receiver order.
float area_sum(std::span<const unsigned> receivers, std::span<const EnvironmentFloor> floors);
// Rejects degenerate or non-finite influence and projection boxes.
bool valid_box(const EnvironmentProbe& probe);
// Prioritises spacing repairs before ordinary high-overlap merges.
bool preferred_pair(const Pair& candidate, const Pair& previous);
// Combines original space diagnostics after a successful union.
void combine_diagnostic(
    EnvironmentSpaceDiagnostic& target, const EnvironmentSpaceDiagnostic& source
);

// Owns merge bookkeeping; scene, floors, rooms and output references outlive this bake operation.
class EnvironmentProbeMerger {
public:
    // Borrows the scene, placement inputs and outputs for one serial merge pass.
    EnvironmentProbeMerger(
        const EnvironmentMergeInputs& inputs,
        std::vector<EnvironmentProbe>& probes,
        std::vector<unsigned>& origins,
        EnvironmentPlacementReport& report
    );

    // Greedy high-overlap pairs are revisited only when one of their captures changes.
    void run();

private:
    // Proximity takes precedence over local GI variation, after two-sided enclosure checks agree.
    std::optional<RoomJoin> join_close_rooms(const Pair& pair);

    // Publishes a repaired union, or restores the domains when no valid spacing solution exists.
    void finish_join(const RoomJoin& change, bool repaired);

    // Tests medium, floor, lighting and overlap eligibility for one active pair.
    std::optional<Pair> candidate_pair(unsigned a, unsigned b) const;

    // Selects the closest spacing conflict, otherwise the highest-overlap pair.
    std::optional<Pair> next_pair() const;

    // Orders supported replacement floors by distance and clearance.
    std::vector<unsigned> candidates(const Pair& pair) const;

    // Ownership checks precede rays. A replacement cannot gain influence in differently lit rooms.
    std::optional<std::vector<unsigned>> support(
        const EnvironmentProbe& probe, unsigned origin
    ) const;

    // Finds a full-coverage replacement, retaining deterministic candidate order.
    Replacement search(const Pair& pair) const;

    // Returns total and pair-local area loss, or rejects unsupported receiver coverage.
    std::optional<std::pair<float, float>> replacement_coverage(
        const ReplacementSearch& search, std::span<const unsigned> accepted
    ) const;

    // Retries contributor orientations and an unrotated box at junctions.
    void consider_orientations(
        ReplacementSearch& search, std::span<const unsigned> locations
    ) const;

    // Tests the bounded subcell lattice in its original deterministic order.
    void consider_subcells(ReplacementSearch& search, std::span<const unsigned> locations) const;

    // Tests one capture position against ownership, coverage and replacement ranking.
    void consider(ReplacementSearch& search, unsigned origin, const EnvironmentFloor& floor) const;

    // Fits one subcell floor position before testing the allowed eye heights.
    void consider_subcell(ReplacementSearch& search, unsigned origin, float dx, float dy) const;

    // If a single capture cannot serve both sides, keep the needed views but move them onto
    // separated supported positions. Every previously covered receiver must remain covered.
    bool repack(const Pair& pair);

    // Finds the best single-capture move while preserving its exclusively owned receivers.
    Replacement relocation(const Pair& pair, unsigned moving);

    // Rejects spacing conflicts except for explicitly excluded active captures.
    bool separated_from_active(
        const EnvironmentProbe& probe, unsigned origin, std::span<const unsigned> excluded
    ) const;

    // Test a moved capture against every unaffected capture before considering paired coverage.
    bool separated_from_others(
        const Pair& pair, const EnvironmentProbe& probe, unsigned origin
    ) const;

    // Updates capture identity, coverage counts and pair generations together.
    void move_capture(unsigned id, const Replacement& location);

    // A wider bounded search is reserved for spacing conflicts that cannot be repaired singly.
    std::vector<Replacement> pair_locations(const Pair& pair, Float3 centre);

    // Ranks separated capture pairs that retain all required receivers.
    std::array<unsigned, 2> choose_pair(
        std::span<const Replacement> locations, std::span<const unsigned> required, Float3 centre
    ) const;

    // Both moves are evaluated together; every previously covered receiver must remain covered.
    bool repack_pair(const Pair& pair);

    // Publishes one merge and records its original capture positions.
    void apply(const Pair& pair, const Replacement& replacement);

    // Removes inactive captures and rebuilds receiver ownership.
    void compact();

    // Remaps original space diagnostics after successful capture-driven fragment unions.
    std::vector<EnvironmentSpaceDiagnostic> room_diagnostics(
        std::span<const unsigned> remap, unsigned count
    ) const;

    // Removes emptied room IDs so runtime validation and placement reports describe the same domains.
    void compact_rooms();

    // Borrowed bake inputs and mutable outputs; their caller owns their lifetime.
    const Scene& m_scene;
    std::span<EnvironmentFloor> m_floors;
    std::vector<std::vector<unsigned>>& m_rooms;
    const BakeSettings& m_settings;
    const EnvironmentBakeSettings& m_options;
    std::vector<EnvironmentProbe>& m_probes;
    std::vector<unsigned>& m_origins;
    EnvironmentPlacementReport& m_report;
    // Owned coverage, generation and union bookkeeping for this merge pass.
    std::vector<bool> m_active;
    std::vector<unsigned> m_versions;
    std::vector<std::vector<unsigned>> m_supported;
    std::vector<unsigned> m_counts;
    std::vector<unsigned> m_room_parent;
    float m_total_area = 0;
    std::set<std::tuple<unsigned, unsigned, unsigned, unsigned>> m_tested;
};

}  // namespace openrac::lighting::bake_detail
