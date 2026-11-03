// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// One admitted export through native preparation, resumable transport and verified publication.
#include <sstream>
#include <stdexcept>

#include "common/profiler.h"
#include "enhanced/tools/baker/batch_command.h"
#include "enhanced/tools/baker/output/report.h"
#include "enhanced/tools/baker/pipeline/bake_json.h"
#include "enhanced/tools/baker/pipeline/coordinator_protocol.h"
#include "enhanced/tools/baker/pipeline/pipeline_internal.h"
#include "enhanced/tools/baker/sources/probe_fingerprint.h"

namespace openrac::lighting::pipeline {
namespace {
// Discards only mismatched cache-owned partials; matching completed grids retain native verification.
void prepare_work(
    const Options& options, const std::filesystem::path& work, const json::Value& identity
) {
    bool same = false;

    // A malformed partial stamp invalidates reuse without trusting its unfinished grids.
    try {
        same = bake_json::equivalent(
            bake_json::document(work / "openrac-bake.partial.json"), identity
        );
    } catch (const std::exception&) {
        same = false;
    }

    // Only mismatched partial work is removed; verified matching grids are retained.
    if (std::filesystem::exists(work) && !same) {
        remove_cache_child(work, options.output);
    }

    std::filesystem::create_directories(work);
    save(work / "openrac-bake.partial.json", identity);
}

// Borrowed per-level execution state, alive until all batch output work has finished.
struct LevelBakeExecution {
    const Options& options;      // Immutable coordinator options.
    BakeThreadPool& pool;        // Shared bounded compute workers.
    ProbeSourceHashes& sources;  // Content snapshots for this level only.
};

// Runs normal batch code directly; there are no native subprocesses or simulated input mailboxes.
int bake(
    const LevelBakeExecution& execution,
    const std::filesystem::path& level,
    const std::filesystem::path& work,
    std::ostream& output
) {
    const auto& options = execution.options;
    std::vector<std::string> arguments{
        "--level-dir",
        level.string(),
        "--batch-output",
        work.string(),
        "--automatic",
        "--resume",
        "--compression",
        probe_texture_token(options.texture_format),
        "--threads",
        std::to_string(options.threads),
        "--profiles-dir",
        options.profiles.string()
    };

    // Native transport and cache identity must share the explicit ray override.
    if (options.rays) {
        arguments.insert(arguments.end(), {"--rays", std::to_string(options.rays)});
    }

    // Zero bounces is a valid direct-only override.
    if (options.bounces >= 0) {
        arguments.insert(arguments.end(), {"--bounces", std::to_string(options.bounces)});
    }

    const auto gameplay = level / "bake-inputs/gameplay.bin";

    const auto settings = region_settings(level);
    const bool analyse = settings["interior_lighting"].boolean() || settings["enabled"].boolean()
                         || profile(level, options).environment.enabled;

    const auto manifest = bake_json::document(level / "manifest.json");

    // Background scenes declare no collision; missing declared floors must still fail preparation.
    if (analyse && !manifest["collision"].is_null() && std::filesystem::is_regular_file(gameplay)) {
        arguments.insert(arguments.end(), {"--prepare-interiors", gameplay.string()});
    }

    std::istringstream no_client;
    return run_batch_command(arguments, no_client, output, &execution.pool, &execution.sources);
}

// Empty transport scenes still need a verified result so subsequent update runs can retain them.
void empty_result(
    const Options& options, const std::filesystem::path& level, const std::filesystem::path& work
) {
    const auto resolved = parsed(describe_level_bake_profile(profile(level, options)));
    save(
        work / "bake-result.json",
        parsed(
            "{\"version\":1,\"verified\":true,\"empty\":true,\"note\":\"nothing to "
            "bake\",\"probe_format_version\":"
            + bake_json::json_text(resolved["probe_format_version"])
            + ",\"bake_version\":" + bake_json::json_text(resolved["bake_version"]) + "}"
        )
    );
}

// Records a controller failure without publishing its partial lighting or stopping other levels.
void report_failure(
    Events& events,
    unsigned id,
    const std::string& name,
    const std::exception& error,
    const std::filesystem::path& log
) {
    const auto detail = coordinator_string(error.what());
    events.line(
        id,
        "openrac-bake-error: {\"version\":1,\"code\":\"native\",\"title\":\"Lighting "
        "preparation failed\",\"detail\":"
            + detail
            + ",\"explanation\":\"The native pipeline could not complete this "
              "level.\",\"suggestion\":\"Check probe-bake.log and the retained batch.log, then "
              "retry.\",\"level\":"
            + coordinator_string(name) + ",\"log\":" + coordinator_string(log.string()) + '}'
    );
    events.finish(
        id,
        parsed(
            "{\"level\":" + coordinator_string(name) + ",\"verified\":false,\"error\":" + detail
            + '}'
        )
    );
}
}  // namespace

void run_level(
    const Options& options, const Level& level, unsigned id, Events& events, BakeThreadPool& pool
) {
    const profiling::Zone timing("Automatic level pipeline");
    const auto name = level.path.filename().string();
    const auto final = options.output / name;

    // A level failure must preserve installed lighting and release admission for other levels.
    try {
        events.line(id, "Bake stage: Prepare sources");
        prepare_sources(options, level.path, pool);
        recover(final);
        ProbeSourceHashes sources{probe_input_hash(level.path), {}};
        const auto identity = stamp(options, level.path, sources.initial);

        // Only a complete matching cache can bypass native transport verification.
        if (options.update && current(final, identity)) {
            events.finish(
                id,
                parsed(
                    "{\"level\":" + coordinator_string(name) + ",\"verified\":true,\"kept\":true}"
                )
            );
            return;
        }

        const auto work = options.output / ('.' + name + ".partial");
        prepare_work(options, work, identity);
        int status = 1;
        // Close all per-level streams before Windows directory publication.
        {
            Output buffer(events, id, work);
            std::ostream output(&buffer);
            output.exceptions(std::ios::badbit | std::ios::failbit);
            status = bake({options, pool, sources}, level.path, work, output);
            buffer.finish();
        }

        // Failed native verification leaves work staged and never publishes its result.
        if (status != 0 && status != 3) {
            throw std::runtime_error(
                "native level bake failed; partial grids and batch.log retained"
            );
        }

        // Sky-only levels still need a verified empty result for cache reuse.
        if (status == 3) {
            empty_result(options, level.path, work);
        }

        // Generated interiors may refresh source identity; commit only the final prepared snapshot.
        save(work / "openrac-bake.json", stamp(options, level.path, sources.verified));
        publish(work, final);
        std::filesystem::remove(final / "openrac-bake.partial.json");
        events.finish(
            id,
            parsed("{\"level\":" + coordinator_string(name) + ",\"verified\":true,\"kept\":false}")
        );
    } catch (const std::exception& error) {
        report_failure(events, id, name, error, options.output / "probe-bake.log");
    }
}
}  // namespace openrac::lighting::pipeline
