// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Native coverage flood post-processing: deterministic height rectangles and source-preserving merges.
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "common/profiler.h"
#include "enhanced/tools/baker/planning/region_plan_internal.h"
#include "enhanced/tools/baker/planning/region_plan_json.h"

namespace openrac::lighting::region_plan {
namespace {
// Restores the original lower/upper heights while merging identical XY coverage cells.
std::vector<Bounds> rectangles(const CoverageResult& result, const Settings& settings) {
    auto remaining = result.cells;
    std::vector<Bounds> bounds;

    // Lexicographic seeds keep rectangle output independent of hash ordering.
    while (!remaining.empty()) {
        const auto [cell, heights] = *remaining.begin();
        const auto [x, y] = cell;
        int right = x + 1;

        // Only equal-height neighbours can share one rectangle.
        while (remaining.contains({right, y}) && remaining.at({right, y}) == heights) {
            ++right;
        }

        int top = y + 1;

        // Grow rows until the next row has a hole or a different height.
        for (;;) {
            bool complete = true;

            // Every column must support the same ceiling band.
            for (int column = x; column < right; ++column) {
                const auto found = remaining.find({column, top});
                complete = complete && found != remaining.end() && found->second == heights;
            }

            // A partial row would add unsupported coverage.
            if (!complete) {
                break;
            }

            ++top;
        }

        // Remove the accepted rectangle before choosing another seed.
        for (int column = x; column < right; ++column) {
            // Each covered cell belongs to exactly one rectangle.
            for (int row = y; row < top; ++row) {
                remaining.erase({column, row});
            }
        }

        const auto& spacing = settings.spacing;
        Bounds box{
            std::max(settings.lower[0], x * spacing[0] * 2 - spacing[0]),
            std::max(settings.lower[1], y * spacing[1] * 2 - spacing[1]),
            heights[0],
            std::min(settings.upper[0], right * spacing[0] * 2 + spacing[0]),
            std::min(settings.upper[1], top * spacing[1] * 2 + spacing[1]),
            heights[1]
        };

        // Clipping can remove a rectangle at the coarse boundary.
        if (box[3] > box[0] && box[4] > box[1] && box[5] > box[2]) {
            bounds.push_back(box);
        }
    }

    return bounds;
}

// Retains every enclosure diagnostic field and the exact reference seed order.
std::string diagnostic(
    unsigned index,
    const Region& region,
    const CoverageResult& result,
    const std::vector<Bounds>& bounds,
    double limit
) {
    std::ostringstream out;
    out << std::setprecision(17) << "{\"seed_region\":" << index
        << ",\"seed_bounds\":" << sequence_json(region.bounds)
        << ",\"indoor_samples\":" << result.indoor
        << ",\"connected_cells_added\":" << result.discovered
        << ",\"disconnected_samples\":" << result.disconnected
        << ",\"limited_surfaces\":" << result.limited << ",\"height_limit\":" << limit
        << ",\"sample_limit_reached\":" << (result.sample_limit_reached ? "true" : "false")
        << ",\"unresolved_or_outdoor_samples\":" << result.seen - result.indoor
        << ",\"changed\":" << (bounds != std::vector<Bounds>{region.bounds} ? "true" : "false")
        << ",\"bounds\":[";

    // Keep coverage pieces in their diagnostic source order.
    for (unsigned part = 0; part < bounds.size(); ++part) {
        if (part) {
            out << ',';
        }

        out << sequence_json(bounds[part]);
    }

    return out.str() + "]}";
}

// Actual supported placement floors are evidence; padded region columns are never anchors.
std::vector<CoverageSeed> coverage_seeds(const State& state) {
    std::vector<CoverageSeed> result;

    // Only original seed provenance may supply flood anchors.
    for (const auto& region : state.regions) {
        CoverageSeed seed;
        seed.bounds = region.bounds;
        // Authored-only reference planning has no combined transition-source list or floor anchors.
        seed.require_anchor = !state.settings.mobys || region.transitions.empty();

        // Supported placements prevent tall padding from attaching to another storey.
        for (const auto& marker : state.markers) {
            // Anchors must belong to this seed and its supported height band.
            if (std::find(region.mobys.begin(), region.mobys.end(), marker.index)
                    != region.mobys.end()
                && marker.ground && region.bounds[2] <= *marker.ground
                && *marker.ground <= region.bounds[5]) {
                seed.anchors.push_back({marker.position[0], marker.position[1], *marker.ground});
            }
        }

        result.push_back(std::move(seed));
    }

    return result;
}

// Combines source IDs when two coverage rectangles describe the same allocation.
void unite_sources(Region& target, const Region& source) {
    std::set<unsigned> mobys(target.mobys.begin(), target.mobys.end());
    mobys.insert(source.mobys.begin(), source.mobys.end());
    target.mobys.assign(mobys.begin(), mobys.end());
    std::set<unsigned> transitions(target.transitions.begin(), target.transitions.end());
    transitions.insert(source.transitions.begin(), source.transitions.end());
    target.transitions.assign(transitions.begin(), transitions.end());
}

// Adds budgeted pieces while retaining provenance on identical allocations.
void append_pieces(
    const Region& region,
    const std::vector<Bounds>& bounds,
    const Settings& settings,
    std::vector<Region>& pieces,
    std::map<Bounds, std::size_t>& indices
) {
    // Coverage rectangles retain the source region metadata.
    for (const auto& box : bounds) {
        // Each piece must fit the per-volume allocation budget.
        for (const auto& part : split_bounds(box, settings)) {
            // Identical allocations combine provenance rather than creating duplicate grids.
            if (const auto found = indices.find(part); found != indices.end()) {
                unite_sources(pieces[found->second], region);
            } else {
                indices.emplace(part, pieces.size());
                pieces.push_back({part, region.mobys, region.transitions});
            }
        }
    }
}

// Converts native flood cells to the same ordered, deduplicated seed pieces used by compaction.
std::vector<Region> coverage_pieces(
    State& state, const Settings& settings, const std::vector<CoverageResult>& results
) {
    std::vector<Region> pieces;
    std::map<Bounds, std::size_t> indices;

    // Preserve each seed diagnostic before deduplicating shared allocations.
    for (unsigned index = 0; index < state.regions.size(); ++index) {
        const auto& region = state.regions[index];
        const auto& covered = results[index];
        const auto bounds =
            covered.indoor ? rectangles(covered, settings) : std::vector<Bounds>{region.bounds};
        state.coverage.push_back(diagnostic(
            index,
            region,
            covered,
            bounds,
            std::min(settings.upper[2], region.bounds[5] + settings.max_rise)
        ));

        append_pieces(region, bounds, settings, pieces, indices);
    }

    return pieces;
}
}  // namespace

void expand_regions(State& state) {
    const profiling::Zone timing("Expand probe region coverage");

    // Disabled or unavailable collision needs no coverage flood.
    if (!state.settings.geometry || !state.geometry_hash || state.regions.empty()) {
        return;
    }

    auto settings = state.settings;

    // Inward clipping keeps expanded endpoints on the supported local lattice.
    for (unsigned axis = 0; axis < 3; ++axis) {
        settings.lower[axis] =
            std::ceil(settings.lower[axis] / settings.spacing[axis]) * settings.spacing[axis];
        settings.upper[axis] =
            std::floor(settings.upper[axis] / settings.spacing[axis]) * settings.spacing[axis];
    }

    CoverageSettings query{
        settings.lower,
        settings.upper,
        settings.spacing,
        settings.max_rise,
        std::max(settings.geometry_padding, settings.spacing[2])
    };
    auto results = extend_surface_coverage(state.triangles, coverage_seeds(state), query);

    // Coverage failures must stop planning before partial plans reach the client.
    if (!results) {
        throw std::runtime_error(results.error().message);
    }

    auto pieces = coverage_pieces(state, settings, *results);
    state.regions = merge_regions(std::move(pieces), settings, true);
    std::sort(state.regions.begin(), state.regions.end(), [](const Region& a, const Region& b) {
        return a.bounds < b.bounds;
    });
}
}  // namespace openrac::lighting::region_plan
