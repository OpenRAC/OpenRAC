// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// final_plan.h's registration boundary: fixed asset names after verified output and source checks.
#include <format>
#include <stdexcept>

#include "common/profiler.h"
#include "enhanced/tools/probes/bake_json.h"
#include "enhanced/tools/probes/batch.h"
#include "enhanced/tools/probes/coordinator_protocol.h"
#include "enhanced/tools/probes/final_plan.h"
#include "enhanced/tools/probes/stage_file.h"

namespace openrac::lighting {
namespace {
// Rejects a missing or empty native output before any verified registration is published.
void require_asset(const std::filesystem::path& path) {
    // Tracing completion alone is insufficient if the expected file was never written.
    if (!std::filesystem::is_regular_file(path) || std::filesystem::file_size(path) == 0) {
        throw std::runtime_error("baker output is missing or empty: " + path.string());
    }
}

// Checks every fixed grid name while constructing the runtime region registry.
std::string region_registry(const ProbeBatchOptions& options, const json::Value& plan) {
    require_asset(options.output_directory / "sparse.orpg");
    std::string regions = "[";

    // Final plan order fixes registry order independently of worker completion.
    for (std::size_t index = 0; index < plan["regions"].items().size(); ++index) {
        const auto name = std::format("region-{:03}.orpg", index);
        require_asset(options.output_directory / name);

        // JSON separators belong between fixed filenames only.
        if (index) {
            regions += ',';
        }

        regions += coordinator_string(name);
    }

    return regions + ']';
}

// Stages each complete report before replacing its existing resume copy.
void publish(const std::filesystem::path& path, const std::string& text) {
    const auto result = write_stage_file(path, text + '\n');

    // Registration failures must propagate instead of emitting the completion marker.
    if (!result) {
        throw std::runtime_error(result.error().message);
    }
}

// Registers only native-verified assets; the result marker is written after both registries.
void register_outputs(const ProbeBatchOptions& options, const json::Value& plan) {
    const auto regions = region_registry(options, plan);
    const auto& report = plan["registration"];
    const auto& profile = report["bake_profile"];
    std::string text =
        "{\"version\":1,\"game_version\":" + bake_json::json_text(plan["coarse"]["game_version"])
        + ",\"level\":" + bake_json::json_text(plan["coarse"]["level"])
        + ",\"scene_hash\":" + bake_json::json_text(plan["scene_hash"])
        + ",\"sparse\":\"sparse.orpg\",\"regions\":" + regions
        + ",\"probe_format_version\":" + bake_json::json_text(profile["probe_format_version"])
        + ",\"bake_version\":" + bake_json::json_text(profile["bake_version"])
        + ",\"verified\":true,\"emission\":" + bake_json::json_text(report["emission"]);

    // Adaptive validation publishes a report only when requested for this completed run.
    if (std::filesystem::is_regular_file(options.output_directory / "probe-density-report.json")) {
        text += ",\"density_report\":\"probe-density-report.json\"";
    }

    // Optional reflection captures are registered only after their stage completed.
    if (std::filesystem::is_regular_file(options.output_directory / "environment.orpe")) {
        text += ",\"environment\":\"environment.orpe\"";
    }

    const std::string registry = "{\"regions\":" + regions + '}';
    publish(options.output_directory / "sparse.orpg.regions.json", registry);
    publish(options.output_directory / "regions.json", registry);
    publish(options.output_directory / "bake-result.json", text + '}');
}
}  // namespace

Result<void> register_batch_outputs(const ProbeBatchOptions& options) {
    // Filesystem errors leave the batch incomplete, retaining partial data for diagnosis.
    try {
        const profiling::Zone timing("Register verified bake outputs");
        json::Value plan;
        std::string error;

        // Registration metadata must come from the same accepted native final plan.
        if (!json::parse(options.plan, plan, error) || !plan["registration"].is_object()) {
            throw std::runtime_error("missing native bake registration metadata");
        }

        register_outputs(options, plan);
        return {};
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}
}  // namespace openrac::lighting
