// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Typed, level-local planning state shared by seed, coverage and integer partition stages.
#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <vector>

#include "common/json.h"
#include "enhanced/lighting/surface_coverage.h"

namespace openrac::lighting::region_plan {
// World-space bounds retain Python's double lattice arithmetic until native grid allocation.
using Bounds = std::array<double, 6>;
// Half-open integer cell coordinates; probe grids include both box endpoints.
using Box = std::array<int, 6>;
using Cell = std::array<int, 3>;

// Hashes lattice membership only; placement decisions still traverse explicitly sorted coordinates.
struct CellHash {
    // Mixes all coordinate bits without narrowing signed world coordinates.
    std::size_t operator()(const Cell& cell) const noexcept {
        // FNV-1a's 64-bit offset and prime provide a portable unsigned coordinate hash.
        std::uint64_t hash = 14'695'981'039'346'656'037ull;

        // Hash traversal has no influence on candidate or output order.
        for (int coordinate : cell) {
            hash ^= static_cast<std::uint32_t>(coordinate);
            hash *= 1'099'511'628'211ull;
        }

        return static_cast<std::size_t>(hash);
    }
};

// Bounds integer occupancy and prefix storage independently of distant source coordinates.
inline constexpr std::size_t kMaximumPlanningCells = 4ull * 1024 * 1024;
using Metrics = std::map<std::string, std::int64_t>;

// One seed or final allocation, with sorted provenance independent of merge completion order.
struct Region {
    Bounds bounds{};                    // World-space bounds before orientation/alignment.
    std::vector<unsigned> mobys;        // Initial placement IDs contributing coverage.
    std::vector<unsigned> transitions;  // Authored boundary IDs contributing coverage.
};

// One whitelisted placement and its supported height bands.
struct Marker {
    unsigned index = 0;            // Initial placement ID.
    unsigned class_id = 0;         // Profile class key.
    std::string name;              // Authored profile label.
    std::string kind;              // Authored profile category.
    CoveragePoint position{};      // Initial world-space placement.
    std::optional<double> ground;  // Closest supported floor below the placement.
    std::optional<double> travel;  // Profile-defined vertical movement, when available.
    Bounds bounds{};               // Original extent before splitting airborne columns.
    std::vector<Bounds> bands;     // Supported lower band and optional airborne band.
};

// Validated numeric controls, owned by one native level controller.
struct Settings {
    CoveragePoint spacing{};          // Local lattice spacing per axis.
    CoveragePoint lower{};            // Coarse supported origin as reported to the client.
    CoveragePoint upper{};            // Coarse supported upper endpoint.
    double padding = 8;               // Authored transition horizontal padding.
    double vertical_padding = 4;      // Authored transition vertical padding.
    double max_rise = 32;             // Confirmed interior ceiling extension limit.
    double geometry_padding = 3;      // Support beyond confirmed interior surfaces.
    unsigned budget = 4096;           // Maximum probes per final allocation.
    unsigned maximum_regions = 1024;  // Runtime region budget.
    bool mobys = true;                // Enable reviewed initial placement seeds.
    bool geometry = true;             // Expand enclosed coverage before compaction.
};

// Mutable planning evidence and reports, never shared between level controllers.
struct State {
    Settings settings;          // Resolved planning controls.
    json::Value profile;        // Resolved transport profile retained in the planning report.
    std::string game_version;   // Export identity checked against the coarse inspection.
    unsigned level = 0;         // Exported level number.
    std::string gameplay_hash;  // Complete retained gameplay SHA-256.
    std::optional<std::string> geometry_hash;   // Present only when collision was needed.
    std::optional<std::string> whitelist_hash;  // Present only when moby seeding was enabled.
    std::vector<Region> regions;                // Current source coverage, then final allocations.
    std::vector<Marker> markers;                // Initial placement evidence.
    std::vector<std::string> skipped;   // JSON records retaining rejected source diagnostics.
    std::vector<std::string> coverage;  // JSON enclosure diagnostics in seed order.
    std::vector<interiors::Triangle>
        triangles;                 // One collision load shared by ground and enclosure.
    unsigned authored_count = 0;   // Authored records, including fog-only boundaries.
    Metrics compaction;            // Coverage-preserving overlap merge counters.
    Metrics repartition;           // Exact integer coverage and blend overlap counters.
    Metrics layout_before;         // Contributor metrics before repartitioning.
    Metrics layout_after;          // Contributor metrics after repartitioning.
    std::vector<Box> final_boxes;  // Exact final endpoints, sorted lexicographically.
};

// Matches Python's nearest-even integer rounding independently of the caller's rounding mode.
int nearest_integer(double value);
// Counts integer cells or endpoint probes; rejects reversed or overflowing boxes.
std::int64_t cell_count(const Box& box);
std::int64_t grid_count(const Box& box);
// Counts the current world box's endpoint probes using the existing nearest-even policy.
std::int64_t probe_count(const Bounds& bounds, const CoveragePoint& spacing);
// Converts aligned world bounds to exact integer cells, refusing off-lattice data.
Box lattice_box(const Bounds& bounds, const CoveragePoint& spacing);
// Mirrors bounded_probe_grid's float snapping before later exact lattice allocation.
Box native_box(const Bounds& bounds, const CoveragePoint& spacing);
// Converts integer endpoints back into double world coordinates without native float snapping.
Bounds world_box(const Box& box, const CoveragePoint& spacing);
// Adds one support cell per side, matching the runtime smoothstep blend border.
Box expanded_box(Box box);
// Visits each cell in deterministic X/Y/Z order with bounded planner storage.
std::vector<Cell> box_cells(const Box& box);
// Partitions oversized world bounds with the former two-cell shared cut band.
std::vector<Bounds> split_bounds(const Bounds& bounds, const Settings& settings);
// Returns contributors at every occupied cell, with zero entries absent.
std::map<Cell, int> contributor_counts(std::span<const Box> boxes);
// Produces the exact existing integer overlap and allocation score.
Metrics layout_metrics(std::span<const Box> boxes, unsigned budget);
// Penalises contributors beyond two, with a 32-point third contributor and eight times squared excess.
std::int64_t blend_cell_cost(int contributors);
// Prices an allocation at one quarter of its probe budget, with a minimum cost of 64.
std::int64_t region_cost(unsigned budget);
// Greedily merges touching boxes with stable source IDs and unchanged pair scoring.
std::vector<Region> merge_regions(
    std::vector<Region> regions,
    const Settings& settings,
    bool preserve_height,
    bool exact_coverage = false
);
// Compacts the original union, then repartitions full-strength cores with one-cell blend aprons.
void partition_regions(State& state);
// Reshapes adjacent cores in deterministic neighbour order under the existing junction cost.
std::vector<Box> refine_regions(
    const std::vector<Box>& boxes, unsigned budget, std::span<const Box> fixed
);
// Builds authored and whitelisted gameplay seeds against one loaded collision snapshot.
void seed_regions(
    State& state, const std::filesystem::path& level, const json::Value& request, bool coverage
);
// Extends interior coverage using the same two-sided native query implementation as the reference path.
void expand_regions(State& state);
// Serializes native planning evidence without transferring collision geometry to the client.
std::string describe_plan(const State& state);
}  // namespace openrac::lighting::region_plan
