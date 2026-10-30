// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The coverage stage decodes one batched request and publishes bounded per-seed result lines.
#include "enhanced/tools/probes/coverage_stage.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <limits>
#include <stdexcept>
#include <utility>

#include "common/json.h"
#include "enhanced/lighting/surface_coverage.h"
#include "enhanced/tools/probes/batch.h"
#include "enhanced/tools/probes/coordinator_protocol.h"
#include "enhanced/tools/probes/final_plan.h"
#include "enhanced/tools/probes/interior_inputs.h"
#include "enhanced/tools/probes/region_plan.h"

namespace openrac::lighting {
namespace {
// Checks finite coordinates before the native flood performs integer lattice conversions.
template <std::size_t Size>
std::array<double, Size> coordinates(const json::Value& value) {
    // Wrong-sized vectors cannot be used as world-space coordinates.
    if (!value.is_array() || value.items().size() != Size) {
        throw std::runtime_error("invalid coverage coordinate count");
    }

    std::array<double, Size> result{};

    // Validate each component before copying into a fixed-size native vector.
    for (std::size_t axis = 0; axis < Size; ++axis) {
        // Missing and nonfinite coordinates must not become zero-valued evidence.
        if (!value[axis].is_number() || !std::isfinite(value[axis].number())) {
            throw std::runtime_error("nonfinite coverage coordinate");
        }

        result[axis] = value[axis].number();
    }

    return result;
}

// Decodes the same resolved settings used by the Python reference coverage implementation.
CoverageSettings read_settings(const json::Value& request) {
    CoverageSettings settings;
    settings.lower = coordinates<3>(request["lower"]);
    settings.upper = coordinates<3>(request["upper"]);
    settings.spacing = coordinates<3>(request["spacing"]);
    settings.max_rise = request["max_rise"].number(0);
    settings.padding = request["padding"].number(-1);
    return settings;
}

// Reads source-floor evidence without shipping the collision triangle soup through JSON.
std::vector<CoverageSeed> read_seeds(const json::Value& request) {
    // Runtime supports at most 1024 final regions; pre-compaction seeds have a separate bounded limit.
    constexpr std::size_t kMaximumSeeds = 16384;

    // Limit pre-compaction input independently of the runtime region budget.
    if (!request["seeds"].is_array() || request["seeds"].items().size() > kMaximumSeeds) {
        throw std::runtime_error("invalid coverage seed count");
    }

    std::vector<CoverageSeed> seeds;

    // Preserve seed order so result indices match the existing Python region list.
    for (const auto& record : request["seeds"].items()) {
        CoverageSeed seed;
        seed.bounds = coordinates<6>(record["bounds"]);
        seed.require_anchor = record["require_anchor"].boolean();

        // Absent floor evidence differs from an explicitly empty list.
        if (!record["anchors"].is_array()) {
            throw std::runtime_error("coverage anchors must be an array");
        }

        // Actual gameplay floors remain the only allowed moby expansion anchors.
        for (const auto& anchor : record["anchors"].items()) {
            seed.anchors.push_back(coordinates<3>(anchor));
        }

        seeds.push_back(std::move(seed));
    }

    return seeds;
}

// Emits one seed at a time, below the coordinator's per-line limit.
void write_result(const CoverageResult& result, std::ostream& output) {
    const auto precision = output.precision();
    output << std::setprecision(std::numeric_limits<double>::max_digits10)
           << "Coverage result {\"indoor\":" << result.indoor << ",\"limited\":" << result.limited
           << ",\"discovered\":" << result.discovered << ",\"disconnected\":" << result.disconnected
           << ",\"seen\":" << result.seen
           << ",\"sample_limit_reached\":" << (result.sample_limit_reached ? "true" : "false")
           << ",\"cells\":[";
    bool comma = false;

    // Sorted cell keys keep native responses deterministic.
    for (const auto& [cell, bounds] : result.cells) {
        // JSON arrays require separators only between elements.
        if (comma) {
            output << ',';
        }

        comma = true;
        output << '[' << cell[0] << ',' << cell[1] << ',' << bounds[0] << ',' << bounds[1] << ']';
    }

    output << "]}\n";
    output.precision(precision);
}

// Runs the complete collision-heavy pass before returning control to Python's box partitioning.
void run_coverage(
    const std::filesystem::path& level, const json::Value& request, std::ostream& output
) {
    const auto triangles = load_coverage_collision(level);

    // Coverage cannot proceed with missing or malformed collision geometry.
    if (!triangles) {
        throw std::runtime_error(triangles.error().message);
    }

    const auto results =
        extend_surface_coverage(*triangles, read_seeds(request), read_settings(request));

    // Malformed lattice or seed settings stop before a final plan can be accepted.
    if (!results) {
        throw std::runtime_error(results.error().message);
    }

    // Per-seed lines keep each coordinator event below its framing limit.
    for (const auto& result : *results) {
        write_result(result, output);
    }

    output << "Coverage complete\n" << std::flush;
}

// Frames complete planning reports below the coordinator's diagnostic-line limit.
void run_region_plan(
    const std::filesystem::path& level, const json::Value& request, std::ostream& output
) {
    const auto result = plan_probe_regions(level, request);

    // Rejected sources cannot produce a partial planning report.
    if (!result) {
        throw std::runtime_error(result.error().message);
    }

    // Escaping a 16 KiB chunk still fits the coordinator's diagnostic-line limit.
    constexpr std::size_t kChunkBytes = 16ull * 1024;

    // The client assembles these bounded records before accepting the complete plan.
    for (std::size_t begin = 0; begin < result->size();) {
        std::size_t end = std::min(begin + kChunkBytes, result->size());

        // Chunks cannot split a UTF-8 profile label across separate JSON records.
        while (end < result->size()
               && (static_cast<unsigned char>((*result)[end]) & 0xC0u) == 0x80u) {
            --end;
        }

        output << "Region plan fragment "
               << coordinator_string(std::string_view(*result).substr(begin, end - begin)) << '\n';
        begin = end;
    }

    output << "Region plan complete\n" << std::flush;
}

// Retains explicit inspection requests and direct transport plans for standalone reference tools.
std::string reference_plan(
    const std::filesystem::path& level,
    const json::Value& request,
    std::istream& input,
    std::ostream& output,
    std::string plan
) {
    // Batches without local coverage can submit the final region plan immediately.
    if (request["type"].string() == "coverage" || request["type"].string() == "region_plan") {
        // Inspection clients can request complete planning or collision coverage alone.
        if (request["type"].string() == "region_plan") {
            run_region_plan(level, request, output);
        } else {
            run_coverage(level, request, output);
        }

        // After coverage, the controller still needs a complete partitioned grid plan.
        if (!read_coordinator_line(input, plan)) {
            throw std::runtime_error("probe batch plan missing after coverage");
        }

        return plan;
    }

    return plan;
}
}  // namespace

Result<std::string> read_covered_plan(
    ProbeBatchOptions& options, std::istream& input, std::ostream& output
) {
    const auto& level = options.level_directory;

    // Protocol and geometry errors leave through the stage Result boundary.
    try {
        std::string plan;

        // Disconnected planners must not leave a level controller waiting indefinitely.
        if (!read_coordinator_line(input, plan)) {
            return unexpected(Error{"probe batch plan missing"});
        }

        json::Value request;
        std::string error;

        // Only complete JSON objects can dispatch the optional coverage stage.
        if (!json::parse(plan, request, error) || !request.is_object()) {
            throw std::runtime_error("invalid coverage/plan request: " + error);
        }

        // Automatic bakes retain the complete report and assemble transport without a client round trip.
        if (request["type"].string() == "bake_level") {
            const auto result = prepare_final_plan(options, request);

            // Registration is enabled only after complete assembly and seed-report publication.
            if (result) {
                options.register_outputs = true;
            }

            return result;
        }

        return reference_plan(level, request, input, output, std::move(plan));
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}
}  // namespace openrac::lighting
