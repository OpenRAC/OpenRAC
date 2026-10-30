// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// automatic_plan.h's source-path and metadata assembly, sharing complete native final-plan validation.
#include "enhanced/tools/probes/automatic_plan.h"

#include <map>
#include <sstream>
#include <stdexcept>

#include "enhanced/tools/probes/bake_json.h"
#include "enhanced/tools/probes/batch.h"
#include "enhanced/tools/probes/coordinator_protocol.h"
#include "enhanced/tools/probes/final_plan.h"
#include "enhanced/tools/probes/pipeline_internal.h"
#include "enhanced/tools/probes/report.h"

namespace openrac::lighting {
namespace {
// Preserves the existing scene inspection's decimal precision for planner lattice decisions.
std::map<std::string, std::string> fields(const std::string& text) {
    std::map<std::string, std::string> result;
    std::istringstream input(text);
    std::string line;

    // Inspection fields retain the same last-value semantics as the reference client.
    while (std::getline(input, line)) {
        const auto split = line.find(' ');

        // Only named inspection fields can contribute metadata.
        if (split != std::string::npos) {
            result[line.substr(0, split)] = line.substr(split + 1);
        }
    }

    return result;
}

// Converts whitespace-separated inspection vectors without another float rounding step.
std::string vector(const std::string& text) {
    std::istringstream stream(text);
    std::string value;
    std::string result = "[";

    // Spacing and bounds retain their original printed precision.
    while (stream >> value) {
        // JSON vector separators belong between components only.
        if (result.size() > 1) {
            result += ',';
        }

        result += value;
    }

    return result + ']';
}
}  // namespace

Result<std::string> automatic_plan(
    const ProbeBatchOptions& options,
    const LevelBakeProfile& profile,
    const std::string& inspection,
    const std::filesystem::path& profiles
) {
    // Malformed inspection or source data must not escape the native planning boundary.
    try {
        const auto data = fields(inspection);
        const auto manifest = bake_json::document(options.level_directory / "manifest.json");
        const auto coarse = pipeline::parsed(
            "{\"game_version\":" + coordinator_string(data.at("game_version"))
            + ",\"level\":" + data.at("level") + ",\"origin\":" + vector(data.at("probe_origin"))
            + ",\"spacing\":" + vector(data.at("probe_spacing"))
            + ",\"dimensions\":" + vector(data.at("probe_dimensions")) + '}'
        );
        bake_json::Object emission;

        // Only the existing emission audit fields belong in the saved seed report.
        for (const auto* key :
             {"emissive_triangles",
              "automatic_liquid_triangles",
              "point_emitters",
              "bake_surfaces",
              "emission",
              "emission_vertex_colour_triangles"}) {
            // Optional audit columns remain absent rather than becoming invented defaults.
            if (data.contains(key)) {
                emission[key] = pipeline::parsed(coordinator_string(data.at(key)));
            }
        }

        const auto gameplay = options.level_directory / "bake-inputs/gameplay.bin";
        const auto whitelist = profiles / manifest["game"].string() / "moby-seeds.json";
        bake_json::Object request{
            {"coarse", coarse},
            {"emission", bake_json::object_value(emission)},
            {"scene_hash",
             pipeline::parsed(coordinator_string(std::to_string(options.scene_hash)))},
            {"profile", pipeline::parsed(describe_level_bake_profile(profile))},
            {"region_settings", pipeline::region_settings(options.level_directory)},
            {"whitelist", pipeline::parsed(coordinator_string(whitelist.string()))},
            {"gameplay",
             pipeline::parsed(coordinator_string(
                 !manifest["collision"].is_null() && std::filesystem::is_regular_file(gameplay)
                     ? gameplay.string()
                     : ""
             ))}
        };
        return prepare_final_plan(options, bake_json::object_value(request));
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}
}  // namespace openrac::lighting
