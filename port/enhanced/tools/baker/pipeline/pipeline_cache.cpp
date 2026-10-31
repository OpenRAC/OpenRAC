// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Semantic native cache identities and recoverable publication of complete verified level directories.
#include <stdexcept>

#include "assets/disc/checksum.h"
#include "common/profiler.h"
#include "enhanced/tools/baker/output/report.h"
#include "enhanced/tools/baker/pipeline/bake_json.h"
#include "enhanced/tools/baker/pipeline/coordinator_protocol.h"
#include "enhanced/tools/baker/pipeline/pipeline_internal.h"
#include "enhanced/tools/baker/pipeline/stage_file.h"
#include "enhanced/tools/baker/sources/interior_stage.h"
#include "enhanced/tools/baker/sources/probe_fingerprint.h"

namespace openrac::lighting::pipeline {
json::Value parsed(const std::string& text) {
    json::Value result;
    std::string error;

    // Generated cache metadata must still parse as complete JSON.
    if (!json::parse(text, result, error)) {
        throw std::runtime_error("invalid pipeline JSON: " + error);
    }

    return result;
}

void save(const std::filesystem::path& path, const json::Value& value) {
    const auto result = write_stage_file(path, bake_json::json_text(value) + '\n');

    // An unclosed or unwritten stamp cannot authorise cache reuse.
    if (!result) {
        throw std::runtime_error(result.error().message);
    }
}

json::Value stamp(
    const Options& options, const std::filesystem::path& level, std::optional<std::uint64_t> source
) {
    const profiling::Zone timing("Compute native cache identity");
    const auto resolved = profile(level, options);
    const auto manifest = bake_json::document(level / "manifest.json");
    const auto whitelist = options.profiles / manifest["game"].string() / "moby-seeds.json";
    const auto settings = region_settings(level);
    const auto moby_hash = (settings["enabled"].boolean() || resolved.environment.enabled)
                                   && std::filesystem::is_regular_file(whitelist)
                               ? coordinator_string(bake_json::file_hash(whitelist))
                               : "null";
    auto description = describe_level_bake_profile(resolved);

    // The launcher's existing profile digest excludes trailing command-line whitespace.
    while (!description.empty() && (description.back() == '\n' || description.back() == '\r')) {
        description.pop_back();
    }

    assets::disc::Sha256 profile_digest;
    profile_digest.update(description);
    return parsed(
        "{\"version\":5,\"pipeline\":1,\"planner\":1,\"environment_algorithm\":20,\"source\":"
        + coordinator_string(std::to_string(source ? *source : probe_input_hash(level)))
        + ",\"manifest_sha256\":"
        + coordinator_string(bake_json::file_hash(level / "manifest.json"))
        + ",\"baker_profile_sha256\":"
        + coordinator_string(assets::disc::to_hex(profile_digest.finish())) + ",\"profile_sha256\":"
        + (std::filesystem::is_regular_file(level / "probe-bake.json")
               ? coordinator_string(bake_json::file_hash(level / "probe-bake.json"))
               : "null")
        + ",\"moby_whitelist_sha256\":" + moby_hash + ",\"profile\":" + description
        + ",\"regions\":" + bake_json::json_text(settings)
        + ",\"interior\":" + interior_stage_profile()
        + ",\"reflections\":" + (resolved.environment.enabled ? "true" : "false") + '}'
    );
}

void update_partial_sources(const std::filesystem::path& output, std::uint64_t source) {
    const auto path = output / "openrac-bake.partial.json";

    // Standalone automatic batches do not own the launcher's partial cache stamp.
    if (!std::filesystem::is_regular_file(path)) {
        return;
    }

    auto fields = bake_json::document(path).members();
    fields["source"] = parsed(coordinator_string(std::to_string(source)));
    save(path, bake_json::object_value(fields));
}

bool current(const std::filesystem::path& output, const json::Value& identity) {
    // Unreadable or incomplete caches are misses and retain their old files until replacement.
    try {
        // Only an identical semantic identity can reuse verified lighting.
        if (!bake_json::equivalent(bake_json::document(output / "openrac-bake.json"), identity)) {
            return false;
        }

        const auto result = bake_json::document(output / "bake-result.json");

        // A retained partial result must never count as completed lighting.
        if (!result["verified"].boolean()) {
            return false;
        }

        // Sky-only scenes are reusable without a sparse grid or reflection asset.
        if (result["empty"].boolean()) {
            return true;
        }

        const auto& names = result["regions"];

        // The registry must remain within the runtime local-volume limit.
        if (!names.is_array() || names.items().size() > 1024) {
            return false;
        }

        std::vector<std::string> assets{"sparse.orpg"};

        // Every registered grid must be present before a level can be retained.
        for (const auto& value : names.items()) {
            const auto& name = value.string();

            // Registry names cannot escape their cache directory.
            if (name.empty() || std::filesystem::path(name).filename() != name
                || !name.ends_with(".orpg")) {
                return false;
            }

            assets.push_back(name);
        }

        // Enabled reflections require their captured environment asset too.
        if (identity["reflections"].boolean()) {
            assets.push_back("environment.orpe");
        }

        // Missing or empty assets invalidate an otherwise matching stamp.
        for (const auto& name : assets) {
            // A verified flag cannot stand in for actual output bytes.
            if (!std::filesystem::is_regular_file(output / name)
                || !std::filesystem::file_size(output / name)) {
                return false;
            }
        }

        return true;
    } catch (const std::exception&) {
        return false;
    }
}

void remove_cache_child(const std::filesystem::path& child, const std::filesystem::path& root) {
    const auto resolved = std::filesystem::weakly_canonical(child);
    const auto parent = std::filesystem::canonical(root);

    // Recursive cleanup is limited to an immediate cache child, after resolving symlinks.
    if (resolved.parent_path() != parent || resolved == parent) {
        throw std::runtime_error("refusing cleanup outside the bake cache");
    }

    std::filesystem::remove_all(resolved);
}

namespace {
// Requires the shared atomic move policy before advancing a publication transaction.
void rename_directory(const std::filesystem::path& source, const std::filesystem::path& target) {
    const auto result = rename_stage_file(source, target);

    // A persistent lock leaves the original transaction state available for rollback or retry.
    if (!result) {
        throw std::runtime_error(result.error().message);
    }
}
}  // namespace

void recover(const std::filesystem::path& final) {
    const auto root = std::filesystem::canonical(final.parent_path());
    const auto previous = root / ('.' + final.filename().string() + ".previous");

    // Recovery may move or remove only an immediate resolved cache child.
    if (std::filesystem::exists(previous)) {
        // Recovery must never move a symlinked backup outside cache ownership.
        if (std::filesystem::weakly_canonical(previous).parent_path() != root) {
            throw std::runtime_error("previous cache directory escapes the cache root");
        }

        // A retained previous directory restores an installation interrupted between renames.
        if (!std::filesystem::exists(final)) {
            rename_directory(previous, final);
        } else {
            remove_cache_child(previous, root);
        }
    }
}

void publish(const std::filesystem::path& work, const std::filesystem::path& final) {
    const profiling::Zone timing("Install verified lighting");
    const auto root = std::filesystem::canonical(final.parent_path());
    const auto previous = root / ('.' + final.filename().string() + ".previous");

    recover(final);

    // Every rename endpoint is resolved inside the same cache before moving any published data.
    if (std::filesystem::weakly_canonical(work).parent_path() != root
        || std::filesystem::weakly_canonical(final).parent_path() != root) {
        throw std::runtime_error("publication paths escape the bake cache");
    }

    const bool moved = std::filesystem::exists(final);

    // The old installation is retained until its replacement is installed.
    if (moved) {
        rename_directory(final, previous);
    }

    // A failed directory swap must preserve the previous published lighting.
    try {
        rename_directory(work, final);
    } catch (const std::exception& error) {
        // Rollback is necessary only after the old directory has moved.
        if (moved) {
            const auto rollback = rename_stage_file(previous, final);

            // A failed restoration reports the retained backup location instead of deleting it.
            if (!rollback) {
                throw std::runtime_error(
                    "installation failed; previous lighting retained at " + previous.string() + ": "
                    + error.what() + "; restoration failed: " + rollback.error().message
                );
            }
        }

        throw;
    }

    // Keep a harmless backup if cleanup is denied after successful installation.
    if (moved) {
        std::error_code ignored;
        std::filesystem::remove_all(previous, ignored);
    }
}
}  // namespace openrac::lighting::pipeline
