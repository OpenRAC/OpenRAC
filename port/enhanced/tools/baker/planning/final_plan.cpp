// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// final_plan.h's final assembly: preserve source reports while deriving native transport bounds.
#include "enhanced/tools/baker/planning/final_plan.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <limits>
#include <stdexcept>
#include <string_view>

#include "common/profiler.h"
#include "enhanced/lighting/probe_density.h"
#include "enhanced/tools/baker/batch.h"
#include "enhanced/tools/baker/pipeline/bake_json.h"
#include "enhanced/tools/baker/pipeline/coordinator_protocol.h"
#include "enhanced/tools/baker/pipeline/stage_file.h"
#include "enhanced/tools/baker/planning/region_plan.h"
#include "enhanced/tools/baker/sources/activation.h"

namespace openrac::lighting {
namespace {
// Parses generated JSON while keeping malformed requests inside the module error boundary.
json::Value parsed(const std::string& text) {
    json::Value value;
    std::string error;

    // Invalid generated or supplied JSON must stop before output identities are replaced.
    if (!json::parse(text, value, error)) {
        throw std::runtime_error("invalid final bake plan: " + error);
    }

    return value;
}

// Converts positive axis spacing without narrowing the original planning arithmetic.
std::array<double, 3> spacing(const json::Value& value) {
    std::array<double, 3> result{};

    // Scalar spacing remains available to authored plans and reference fixtures.
    if (value.is_number()) {
        result.fill(value.number());
    } else {
        // Anisotropic volumes require exactly three axes.
        if (!value.is_array() || value.items().size() != result.size()) {
            throw std::runtime_error("invalid final region spacing");
        }

        // Every spacing component is validated before its distance cap is calculated.
        for (std::size_t axis = 0; axis < result.size(); ++axis) {
            // Missing components cannot silently become zero.
            if (!value[axis].is_number()) {
                throw std::runtime_error("invalid final region spacing");
            }

            result[axis] = value[axis].number();
        }
    }

    // Nonpositive or overflowing axes cannot define usable grids or visibility distances.
    for (double axis : result) {
        // Every native grid needs a finite positive step.
        if (!std::isfinite(axis) || axis <= 0) {
            throw std::runtime_error("invalid final region spacing");
        }
    }

    return result;
}

// Checks the shared planner boundary before any cached output identities can change.
json::Value planned_seeds(
    const ProbeBatchOptions& options, const json::Value& request, RegionPlanPurpose purpose
) {
    const auto result = plan_probe_regions(options.level_directory, request, purpose);

    // Planner errors must leave previous resume identities untouched.
    if (!result) {
        throw std::runtime_error(result.error().message);
    }

    return parsed(*result);
}

// Keeps floor evidence available independently of local allocations and generated interior lights.
json::Value seed_plan(const ProbeBatchOptions& options, const json::Value& request) {
    const bool coverage = request["region_settings"]["enabled"].boolean();
    const bool reflections = load_level_bake_profile(options.level_directory).environment.enabled;

    // Sparse-only scenes need no gameplay when neither placement feature uses it.
    if (!coverage && !reflections) {
        return parsed(
            R"({"regions":[],"note":"Dense regions disabled by the per-level profile."})"
        );
    }

    // Disc transition scenes have no gameplay to seed local volumes.
    if (request["gameplay"].string().empty()) {
        return parsed(
            R"({"regions":[],"note":"No gameplay to seed dense regions from (a scene, not a level of the disc)."})"
        );
    }

    auto inputs = request.members();
    inputs["options"] = request["region_settings"];
    const auto query = bake_json::object_value(inputs);
    // Local allocation policy must not control reflection anchor density or availability.
    const auto purpose = coverage ? RegionPlanPurpose::Coverage : RegionPlanPurpose::FloorAnchors;
    auto plan = planned_seeds(options, query, purpose);

    // Disabling moby GI seeds must not remove the independent reflection floor anchors.
    if (coverage && reflections && plan["moby_seeds"].items().empty()) {
        const auto anchors = planned_seeds(options, query, RegionPlanPurpose::FloorAnchors);
        auto fields = plan.members();
        fields["floor_anchors"] = anchors["moby_seeds"];
        plan = bake_json::object_value(fields);
    }

    return plan;
}

// Recognises only fixed baker filenames, leaving similarly named private files untouched.
bool grid_output(std::string_view name) {
    // The derived registry belongs to the sparse grid alongside its encoded payload and report.
    if (name == "sparse.orpg.regions.json") {
        return true;
    }

    const auto dot = name.find('.');
    const auto stem = name.substr(0, dot);

    // Local grids use numeric IDs; arbitrary filenames containing .orpg are not owned outputs.
    if (stem != "sparse") {
        const auto number = stem.starts_with("region-") ? stem.substr(7) : std::string_view{};

        if (number.empty() || !std::all_of(number.begin(), number.end(), [](char digit) {
                return digit >= '0' && digit <= '9';
            })) {
            return false;
        }
    }

    // Only these payload/report staging suffixes are produced by the grid writer.
    const auto suffix = name.substr(std::min(dot, name.size()));
    return suffix == ".orpg" || suffix == ".orpg.txt" || suffix == ".orpg.tmp"
           || suffix == ".orpg.txt.tmp";
}

// Removes only grid-owned artifacts when the complete resume identity changes.
void remove_stale_grids(const ProbeBatchOptions& options, const json::Value& plan) {
    const auto path = options.output_directory / "seed-plan.json";

    // A fresh directory has no grid identity to compare.
    if (!std::filesystem::exists(path)) {
        return;
    }

    // Fresh bakes cannot replace an earlier plan or its output files.
    if (!options.resume) {
        throw std::runtime_error("seed plan already exists; choose a new output directory");
    }

    bool same = false;

    // An interrupted or malformed old report invalidates its grids, as a changed plan does.
    try {
        same = bake_json::equivalent(bake_json::document(path), plan);
    } catch (const std::runtime_error&) {
        same = false;
    }

    // Equivalent parsed values preserve finished grids despite JSON formatting differences.
    if (same) {
        return;
    }

    // Only fixed grid outputs are invalidated; diagnostics and private files remain.
    for (const auto& entry : std::filesystem::directory_iterator(options.output_directory)) {
        // Fixed grid/report suffixes are the only permitted invalidation targets.
        if (entry.is_regular_file() && grid_output(entry.path().filename().string())) {
            std::filesystem::remove(entry.path());
        }
    }
}

// Produces the exact per-axis padded union without changing geometry or ray-distance bounds.
std::string coarse_bounds(const json::Value& regions, const json::Value& settings) {
    // Scene mode and empty local coverage retain the original complete scene allocation.
    if (settings["coarse_bounds"].string() == "scene" || regions.items().empty()) {
        return {};
    }

    const double padding = settings["coarse_padding"].number(-1);

    std::array<double, 6> bounds;
    bounds.fill(std::numeric_limits<double>::infinity());
    std::fill(bounds.begin() + 3, bounds.end(), -std::numeric_limits<double>::infinity());

    // Every local volume contributes its complete unaligned world bounds.
    for (const auto& region : regions.items()) {
        // Three independent axis unions preserve the original double arithmetic.
        for (std::size_t axis = 0; axis < 3; ++axis) {
            bounds[axis] = std::min(bounds[axis], region["bounds"][axis].number());
            bounds[axis + 3] = std::max(bounds[axis + 3], region["bounds"][axis + 3].number());
        }
    }

    return std::format(
        "[{}, {}, {}, {}, {}, {}]",
        bounds[0] - padding,
        bounds[1] - padding,
        bounds[2] - padding,
        bounds[3] + padding,
        bounds[4] + padding,
        bounds[5] + padding
    );
}

// Serialises fixed transport fields without returning the potentially large report to Python.
std::string transport_regions(const json::Value& plan) {
    std::string result = "[";

    // Planned order determines runtime volume filenames and atlas ordering.
    for (const auto& region : plan["regions"].items()) {
        const auto axes = spacing(region["spacing"]);
        // Distance cap stays at 1.5 cell diagonals, including relocation allowance.
        const double distance =
            1.5 * std::sqrt((axes[0] * axes[0] + axes[1] * axes[1]) + axes[2] * axes[2]);
        bake_json::Object fields{{"bounds", region["bounds"]}};
        fields["spacing"] = parsed(std::format("[{},{},{}]", axes[0], axes[1], axes[2]));
        fields["max_distance"] = parsed(std::format("{}", distance));

        // Repartitioned integer bounds must survive without another float-grid expansion.
        if (!region["lattice_bounds"].is_null()) {
            fields["lattice_bounds"] = region["lattice_bounds"];
        }

        // JSON elements are separated without a trailing comma.
        if (result.size() > 1) {
            result += ',';
        }

        result += bake_json::json_text(bake_json::object_value(fields));
    }

    return result + ']';
}

// Rejects invalid registration and adaptive settings before invalidating any completed grids.
void validate_metadata(const json::Value& request) {
    const auto& settings = request["region_settings"];
    const auto& profile = request["profile"];
    const auto regions = coordinator_integer(settings["max_regions"], 1024);
    const auto& policy = settings["coarse_bounds"].string();
    const double padding = settings["coarse_padding"].number(-1);

    // Allocation policy is validated even when there are no local regions.
    if ((policy != "scene" && policy != "local_regions") || !std::isfinite(padding)
        || padding < 0) {
        throw std::runtime_error("invalid coarse allocation policy");
    }

    // Runtime limits and required profile fields cannot be absent in a completed registration.
    if (!regions || !coordinator_integer(profile["probe_format_version"], 1'000'000)
        || !coordinator_integer(profile["bake_version"], 1'000'000)
        || request["coarse"]["game_version"].string().empty()) {
        throw std::runtime_error("invalid automatic bake metadata");
    }

    coordinator_integer(request["coarse"]["level"], 1'000'000);

    // Missing numeric limits must not silently become a different adaptive policy.
    if (settings["adaptive_density"].type() != json::Value::Type::Bool
        || !settings["adaptive_relative_error"].is_number()
        || !settings["adaptive_absolute_error"].is_number()) {
        throw std::runtime_error("invalid automatic density policy");
    }

    bake_detail::ProbeDensitySettings density;
    density.enabled = settings["adaptive_density"].boolean();
    density.relative_error = static_cast<float>(settings["adaptive_relative_error"].number());
    density.absolute_error = static_cast<float>(settings["adaptive_absolute_error"].number());
    density.validate();
}

// Attaches the same settings, source identity and emission audit to the saved planner report.
json::Value complete_report(const ProbeBatchOptions& options, const json::Value& request) {
    const auto& settings = request["region_settings"];

    // Required metadata must be complete before any resume invalidation or output write.
    if (!settings.is_object() || settings["enabled"].type() != json::Value::Type::Bool
        || !request["profile"].is_object() || !request["emission"].is_object()
        || request["scene_hash"].string() != std::to_string(options.scene_hash)) {
        throw std::runtime_error("incomplete automatic bake request");
    }

    validate_metadata(request);
    auto fields = seed_plan(options, request).members();
    const auto count = fields.at("regions").items().size();

    // Runtime capacity and the configured level cap both bound final allocations.
    if (count > coordinator_integer(settings["max_regions"], 1024)) {
        throw std::runtime_error("planned dense regions exceed the runtime region limit");
    }

    fields["region_settings"] = settings;
    fields["bake_profile"] = request["profile"];
    fields["scene_hash"] = request["scene_hash"];
    fields["emission"] = request["emission"];
    return bake_json::object_value(fields);
}
}  // namespace

Result<std::string> prepare_final_plan(
    const ProbeBatchOptions& options, const json::Value& request
) {
    // Source, protocol and filesystem failures cross the public boundary as one error.
    try {
        const profiling::Zone timing("Assemble final bake plan");
        const auto report = complete_report(options, request);
        const auto& settings = request["region_settings"];
        const auto& regions = report["regions"];
        const auto bounds = coarse_bounds(regions, settings);
        const auto density = parsed(
            "{\"enabled\":" + bake_json::json_text(settings["adaptive_density"])
            + ",\"relative_error\":" + bake_json::json_text(settings["adaptive_relative_error"])
            + ",\"absolute_error\":" + bake_json::json_text(settings["adaptive_absolute_error"])
            + "}"
        );
        bake_json::Object fields{
            {"version", parsed("1")},
            {"scene_hash", request["scene_hash"]},
            {"regions", parsed(transport_regions(report))},
            {"adaptive_density", density},
            {"registration",
             bake_json::object_value(
                 {{"bake_profile", request["profile"]}, {"emission", request["emission"]}}
             )},
            {"coarse", request["coarse"]}
        };

        // Coarse bounds are optional, preserving the scene-wide fallback for empty local plans.
        if (!bounds.empty()) {
            fields["coarse_bounds"] = parsed(bounds);
        }

        const auto transport = bake_json::json_text(bake_json::object_value(fields));
        remove_stale_grids(options, report);
        const auto written = write_stage_file(
            options.output_directory / "seed-plan.json", bake_json::json_text(report) + '\n'
        );

        // A failed report write cannot authorise dependent grid baking.
        if (!written) {
            throw std::runtime_error(written.error().message);
        }

        return transport;
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}
}  // namespace openrac::lighting
