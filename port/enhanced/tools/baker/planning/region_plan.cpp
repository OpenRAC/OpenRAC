// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Region planning orchestration, independent of transport and graphics resources.
#include "enhanced/tools/baker/planning/region_plan.h"

#include <cmath>
#include <stdexcept>

#include "common/profiler.h"
#include "enhanced/tools/baker/pipeline/coordinator_protocol.h"
#include "enhanced/tools/baker/planning/region_plan_internal.h"
#include "enhanced/tools/baker/planning/region_plan_json.h"

namespace openrac::lighting {
namespace {
// Reads a finite optional scalar without treating strings or booleans as numeric controls.
double setting(const json::Value& options, const char* key, double fallback) {
    const auto& value = options[key];

    // Missing controls inherit the established planning defaults.
    if (value.is_null()) {
        return fallback;
    }

    // Reject malformed controls before arithmetic or allocation.
    if (!value.is_number() || !std::isfinite(value.number()) || value.number() < 0) {
        throw std::runtime_error("invalid numeric region planning option");
    }

    return value.number();
}

// Preserves explicit opt-outs while rejecting wrongly typed protocol values.
bool enabled(const json::Value& options, const char* key) {
    const auto& value = options[key];

    // Absent opt-outs leave automatic planning enabled.
    if (value.is_null()) {
        return true;
    }

    // Protocol switches must not silently accept numbers or strings.
    if (value.type() != json::Value::Type::Bool) {
        throw std::runtime_error("invalid region planning switch");
    }

    return !value.boolean();
}

// Resolves and validates all grid axes before any lattice arithmetic.
void resolve_axes(region_plan::Settings& result, const json::Value& request) {
    const auto& coarse = request["coarse"];
    const auto& options = request["options"];
    result.spacing = region_plan::coordinates<3>(request["profile"]["dense_spacing"]);
    result.lower = region_plan::coordinates<3>(coarse["origin"]);
    const auto coarse_spacing = region_plan::coordinates<3>(coarse["spacing"]);
    const auto dimensions = region_plan::coordinates<3>(coarse["dimensions"]);

    // Resolve per-axis overrides before checking the coarse-grid relationship.
    for (unsigned axis = 0; axis < 3; ++axis) {
        // Horizontal overrides affect both floor-plane axes.
        if (!options["spacing"].is_null() && axis < 2) {
            result.spacing[axis] = setting(options, "spacing", 0);
        }

        // Vertical density has a separate override.
        if (!options["vertical_spacing"].is_null() && axis == 2) {
            result.spacing[axis] = setting(options, "vertical_spacing", 0);
        }

        // Local grids must be strictly finer than the coarse fallback.
        if (result.spacing[axis] <= 0 || result.spacing[axis] >= coarse_spacing[axis]
            || dimensions[axis] < 2 || std::floor(dimensions[axis]) != dimensions[axis]) {
            throw std::runtime_error(
                "region spacing must be positive and finer than the level grid"
            );
        }

        result.upper[axis] = result.lower[axis] + coarse_spacing[axis] * (dimensions[axis] - 1);

        // Reject overflow and float underflow before native grid allocation.
        if (!std::isfinite(result.upper[axis])
            || !std::isfinite(static_cast<float>(result.spacing[axis]))
            || static_cast<float>(result.spacing[axis]) <= 0) {
            throw std::runtime_error("region planning extent or spacing exceeds native limits");
        }
    }
}

// Resolves profile and explicit controls without changing the reference defaults.
region_plan::Settings settings(const json::Value& request) {
    region_plan::Settings result;
    resolve_axes(result, request);
    const auto& options = request["options"];
    // Padding and rise defaults are world units; the minimum budget includes a blend apron.
    result.padding = setting(options, "padding", 8);
    result.vertical_padding = setting(options, "vertical_padding", result.padding);
    result.max_rise = setting(options, "geometry_max_rise", 32);
    result.geometry_padding = setting(options, "geometry_padding", 3);
    result.budget = static_cast<unsigned>(coordinator_integer(options["max_probes"], 1'000'000));
    result.maximum_regions =
        static_cast<unsigned>(coordinator_integer(options["max_regions"], 1024));
    result.mobys = enabled(options, "no_moby_seeds");
    result.geometry = enabled(options, "no_geometry_coverage");

    // Budgets must leave room for a minimal blended allocation.
    if (result.budget < 64 || result.maximum_regions < 1 || result.max_rise <= 0) {
        throw std::runtime_error("invalid region probe budget or coverage limit");
    }

    return result;
}
}  // namespace

Result<std::string> plan_probe_regions(
    const std::filesystem::path& level, const json::Value& request, RegionPlanPurpose purpose
) {
    // Convert malformed source and planning errors into the public result type.
    try {
        const profiling::Zone timing("Plan probe regions");
        const auto manifest = region_plan::document(level / "manifest.json");
        region_plan::State state;
        const bool coverage = purpose == RegionPlanPurpose::Coverage;

        // Floor anchors need no local-grid density, clipping or allocation policy.
        if (coverage) {
            state.settings = settings(request);
        } else {
            state.settings.spacing.fill(1);
            state.settings.geometry = false;
        }
        state.profile = request["profile"];
        state.game_version = manifest["game"].string() + '-' + manifest["version"].string();
        state.level = static_cast<unsigned>(coordinator_integer(manifest["level"], 1'000'000));

        // Never combine source evidence from a different level.
        if (request["coarse"]["game_version"].string() != state.game_version
            || request["coarse"]["level"].number(-1) != state.level) {
            throw std::runtime_error("sparse asset and level identity differ");
        }

        region_plan::seed_regions(state, level, request, coverage);

        // Disabled local grids retain gameplay evidence without running coverage or partition stages.
        if (coverage) {
            region_plan::expand_regions(state);
            region_plan::partition_regions(state);
        }

        // The runtime cannot consume plans beyond its region budget.
        if (state.regions.size() > state.settings.maximum_regions) {
            throw std::runtime_error(
                "planned dense regions exceed the runtime region limit; adjust the level planning "
                "settings"
            );
        }

        return region_plan::describe_plan(state);
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}
}  // namespace openrac::lighting
