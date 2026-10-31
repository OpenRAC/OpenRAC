// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Native source validation, declarative water decoding and shared emission preparation.
#include <chrono>
#include <format>
#include <stdexcept>

#include "common/profiler.h"
#include "enhanced/tools/baker/pipeline/bake_json.h"
#include "enhanced/tools/baker/pipeline/coordinator_protocol.h"
#include "enhanced/tools/baker/pipeline/pipeline_internal.h"
#include "enhanced/tools/baker/pipeline/stage_file.h"
#include "enhanced/tools/baker/sources/emission_prepare.h"
#include "enhanced/tools/water/water_bake.h"
#include "enhanced/tools/water/water_source.h"

namespace openrac::lighting::pipeline {
namespace {
// Owns a uniquely created water staging directory; cleanup never traverses unrelated children.
class WaterStage {
public:
    explicit WaterStage(const std::filesystem::path& level) {
        directory =
            level
            / (".water-native-"
               + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));

        // Exclusive creation prevents one water writer from cleaning another writer staging files.
        if (!std::filesystem::create_directory(directory)) {
            throw std::runtime_error("cannot stage native water");
        }
    }

    ~WaterStage() {
        std::error_code ignored;

        // Cleanup touches only filenames owned by this staging object.
        for (const auto* name : {"input.json", "water.bin", "water.mesh"}) {
            std::filesystem::remove(directory / name, ignored);
        }

        std::filesystem::remove(directory, ignored);
    }

    WaterStage(const WaterStage&) = delete;
    WaterStage& operator=(const WaterStage&) = delete;
    WaterStage(WaterStage&&) = delete;
    WaterStage& operator=(WaterStage&&) = delete;
    std::filesystem::path directory;  // Exclusively owned, inside the source level.
};

// Verifies both members before deciding that an existing native water pair is reusable.
bool water_current(const std::filesystem::path& level, const json::Value& inputs) {
    // An incomplete water pair is a cache miss, never a trusted partial result.
    try {
        const auto stamp = bake_json::document(level / "water-bake.json");
        return stamp["version"].number() == 3 && bake_json::equivalent(stamp["inputs"], inputs)
               && stamp["outputs"][0].string() == bake_json::file_hash(level / "water.bin")
               && stamp["outputs"][1].string() == bake_json::file_hash(level / "water.mesh");
    } catch (const std::exception&) {
        return false;
    }
}

// Keeps the existing byte-for-byte water formats and fingerprints supplied by source decoding.
void prepare_water(const std::filesystem::path& level) {
    const profiling::Zone timing("Prepare native water");
    const auto boundaries = level / "water-boundaries.json";
    const auto inputs = parsed(
        "{\"algorithm\":\"native-water-sources-v1\",\"overlay\":"
        + coordinator_string(bake_json::file_hash(level / "bake-inputs/overlay.bin"))
        + ",\"profile\":" + coordinator_string(bake_json::file_hash(level / "probe-bake.json"))
        + ",\"collision\":" + coordinator_string(bake_json::file_hash(level / "collision.glb"))
        + ",\"boundaries\":"
        + (std::filesystem::is_regular_file(boundaries)
               ? coordinator_string(bake_json::file_hash(boundaries))
               : "null")
        + '}'
    );

    // Unchanged source identity and both output hashes avoid repeating numerical water work.
    if (water_current(level, inputs)) {
        return;
    }

    const auto decoded = water::decode_sources(level);

    // Bad source layouts cannot publish numerical outputs or a completed cache stamp.
    if (!decoded) {
        throw std::runtime_error(decoded.error().message);
    }

    WaterStage stage(level);
    save(stage.directory / "input.json", *decoded);
    const auto result = water::bake(stage.directory / "input.json", stage.directory / "water.bin");

    // Failed water numerics cannot replace either published member.
    if (!result) {
        throw std::runtime_error(result.error().message);
    }

    // The shared fingerprint protects consumers if publication is interrupted between files.
    for (const auto* name : {"water.bin", "water.mesh"}) {
        const auto replaced = replace_stage_file(stage.directory / name, level / name);

        // A failed replacement must not publish a completed water stamp.
        if (!replaced) {
            throw std::runtime_error(replaced.error().message);
        }
    }

    save(
        level / "water-bake.json",
        parsed(
            "{\"version\":3,\"inputs\":" + bake_json::json_text(inputs) + ",\"outputs\":[\""
            + bake_json::file_hash(level / "water.bin") + "\",\""
            + bake_json::file_hash(level / "water.mesh") + "\"]}"
        )
    );
}
}  // namespace

void prepare_sources(
    const Options& options, const std::filesystem::path& level, BakeThreadPool& pool
) {
    const profiling::Zone timing("Prepare exported bake sources");
    const auto manifest = bake_json::document(level / "manifest.json");
    const auto profile = bake_json::optional_object(level / "probe-bake.json");
    const bool water = profile.contains("water_sources");

    // Exports declaring water must carry the matching tracked decoder profile.
    if (!manifest["bake_inputs"]["water"].is_null() && !water) {
        throw std::runtime_error("missing water_sources in probe-bake.json; re-export the level");
    }

    const auto settings = region_settings(level);
    const bool gameplay = settings["enabled"].boolean() || settings["interior_lighting"].boolean()
                          || load_level_bake_profile(level).environment.enabled;

    // Current exports retain gameplay inputs; older exports must be regenerated.
    if (gameplay && !manifest["collision"].is_null()
        && !std::filesystem::is_regular_file(level / "bake-inputs/gameplay.bin")) {
        throw std::runtime_error(
            "missing bake-inputs/gameplay.bin; re-export the level before baking"
        );
    }

    // Only exports declaring water tables need the native water stage.
    if (water) {
        prepare_water(level);
    }

    const auto identity = manifest["game"].string() + '-' + manifest["version"].string();
    const auto directory = options.profiles / identity;
    const auto result = prepare_level_emission(
        level,
        directory / "emission-materials.json",
        directory / std::format("level_{:02}", static_cast<int>(manifest["level"].number()))
            / "probe-emission.json",
        {options.threads, &pool}
    );

    // Invalid material preparation cannot enter a verified scene snapshot.
    if (!result) {
        throw std::runtime_error(result.error().message);
    }

    // Explicit opt-outs remove only generated interiors, preserving independent authored sources.
    if (!region_settings(level)["interior_lighting"].boolean()
        && std::filesystem::exists(level / "probe-interiors.json")
        && bake_json::document(level / "probe-interiors.json")["generator"].string()
               == "openrac-interiors-v1") {
        std::filesystem::remove(level / "probe-interiors.json");
    }
}
}  // namespace openrac::lighting::pipeline
