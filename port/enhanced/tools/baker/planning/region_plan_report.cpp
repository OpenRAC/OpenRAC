// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Region planning evidence reports shared by native automatic and standalone validation flows.
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include "enhanced/tools/baker/pipeline/coordinator_protocol.h"
#include "enhanced/tools/baker/planning/region_plan_internal.h"
#include "enhanced/tools/baker/planning/region_plan_json.h"

namespace openrac::lighting::region_plan {
namespace {
// Diagnostic counters are exact integers rather than floating-point score estimates.
std::string metrics_json(const Metrics& metrics) {
    std::string result = "{";

    // Sorted counter names make reports stable across runs.
    for (const auto& [name, value] : metrics) {
        // JSON separators belong between counters only.
        if (result.size() > 1) {
            result += ',';
        }

        result += coordinator_string(name) + ':' + std::to_string(value);
    }

    return result + '}';
}

// Nullable source identities distinguish unavailable evidence from empty files.
std::string optional_text(const std::optional<std::string>& value) {
    return value ? coordinator_string(*value) : "null";
}

// Marker records retain supported floor and piston travel evidence without introducing bake geometry.
std::string marker_json(const Marker& marker) {
    std::ostringstream out;
    out << std::setprecision(17) << "{\"index\":" << marker.index
        << ",\"class\":" << marker.class_id << ",\"name\":" << coordinator_string(marker.name)
        << ",\"kind\":" << coordinator_string(marker.kind)
        << ",\"position\":" << sequence_json(marker.position) << ",\"ground_z\":";

    // Unavailable floor evidence remains distinct from a floor at zero.
    if (marker.ground) {
        out << *marker.ground;
    } else {
        out << "null";
    }

    out << ",\"piston_travel_z\":";

    // Static placements have no inferred travel distance.
    if (marker.travel) {
        out << *marker.travel;
    } else {
        out << "null";
    }

    out << ",\"bounds\":" << sequence_json(marker.bounds) << ",\"coverage_bounds\":[";

    // Airborne placements retain both supported height bands.
    for (unsigned index = 0; index < marker.bands.size(); ++index) {
        // JSON separators belong between bands only.
        if (index) {
            out << ',';
        }

        out << sequence_json(marker.bands[index]);
    }

    // Multiple bands mean the airborne item did not create one tall floor-to-item column.
    out << "],\"split_airborne_column\":" << (marker.bands.size() > 1 ? "true" : "false") << '}';
    return out.str();
}

// Pre-serialized records remain separate so large levels can be framed in bounded control-channel chunks.
std::string records_json(const std::vector<std::string>& records) {
    std::string result = "[";

    // Keep source diagnostic order intact.
    for (const auto& record : records) {
        // JSON separators belong between records only.
        if (result.size() > 1) {
            result += ',';
        }

        result += record;
    }

    return result + ']';
}

// Keeps optional layout diagnostics separate from allocation serialization.
std::string repartition_json(const State& state) {
    auto repartition = metrics_json(state.repartition);

    // Empty plans have no before-and-after overlap measurements.
    if (!state.layout_before.empty()) {
        repartition.pop_back();
        repartition += ",\"layout_before\":" + metrics_json(state.layout_before)
                       + ",\"layout_after\":" + metrics_json(state.layout_after) + '}';
    }

    return repartition;
}

// Reports packed atlas residency without allocating map storage.
std::int64_t resident_bytes(const State& state, std::int64_t capacity) {
    // Six interior texels are the default map resolution.
    const double resolution = state.profile["resolution"].number(6);

    // Validate before narrowing even when a malformed client bypasses the profile reader.
    if (!std::isfinite(resolution) || resolution < 1 || resolution > 64
        || std::floor(resolution) != resolution) {
        throw std::runtime_error("invalid region planning probe-map resolution");
    }

    // Two border texels surround each map; packed maps use 12 bytes per texel plus 8 per probe.
    const auto side = static_cast<std::int64_t>(resolution) + 2;
    return capacity * (12 * side * side + 8);
}
}  // namespace

std::string describe_plan(const State& state) {
    std::ostringstream out;
    out << std::setprecision(17) << "{\"level\":" << state.level
        << ",\"game_version\":" << coordinator_string(state.game_version)
        << ",\"source\":\"authored environment transitions (lighting bit 0) and whitelisted "
           "initial moby placements\""
        << ",\"gameplay_sha256\":" << coordinator_string(state.gameplay_hash)
        << ",\"bake_profile\":" << json_text(state.profile)
        << ",\"authored_count\":" << state.authored_count
        << ",\"moby_whitelist_sha256\":" << optional_text(state.whitelist_hash)
        << ",\"moby_seeds\":[";

    // Reviewed placement evidence remains in original source order.
    for (unsigned index = 0; index < state.markers.size(); ++index) {
        // JSON separators belong between placements only.
        if (index) {
            out << ',';
        }

        out << marker_json(state.markers[index]);
    }

    out << "],\"regions\":[";
    std::int64_t capacity = 0;

    // Final region order also determines atlas allocation order.
    for (unsigned index = 0; index < state.regions.size(); ++index) {
        // JSON separators belong between allocations only.
        if (index) {
            out << ',';
        }

        const auto& region = state.regions[index];
        const auto& box = state.final_boxes[index];
        const auto count = grid_count(box);
        // Each atlas row allocates capacity in groups of 64 probes.
        capacity += ((count + 63) / 64) * 64;
        out << "{\"bounds\":" << sequence_json(region.bounds)
            << ",\"lattice_bounds\":" << sequence_json(box)
            << ",\"spacing\":" << sequence_json(state.settings.spacing)
            << ",\"probe_count\":" << count << ",\"source_mobys\":" << sequence_json(region.mobys)
            << ",\"source_transitions\":" << sequence_json(region.transitions) << '}';
    }

    const auto repartition = repartition_json(state);

    const auto bytes = resident_bytes(state, capacity);
    out << "],\"skipped\":" << records_json(state.skipped)
        << ",\"geometry_coverage\":" << records_json(state.coverage)
        << ",\"region_compaction\":" << metrics_json(state.compaction)
        << ",\"region_repartition\":" << repartition << ",\"dense_resident_bytes\":" << bytes
        << ",\"dense_residency_budget_exceeded\":"
        << (bytes > 256ll * 1024 * 1024 ? "true" : "false")
        << ",\"geometry_sha256\":" << optional_text(state.geometry_hash)
        << ",\"note\":\"Light-bank IDs are seed evidence; bake transport uses the sparse bake "
           "lighting inputs.\"}";
    return out.str();
}
}  // namespace openrac::lighting::region_plan
