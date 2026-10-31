// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// pipeline.h's end-to-end native controller; source decoding and numerical work stay in-process.
#include "enhanced/tools/baker/pipeline/pipeline.h"

#include <algorithm>
#include <format>
#include <stdexcept>

#include "common/profiler.h"
#include "enhanced/tools/baker/pipeline/bake_json.h"
#include "enhanced/tools/baker/pipeline/coordinator.h"
#include "enhanced/tools/baker/pipeline/coordinator_protocol.h"
#include "enhanced/tools/baker/pipeline/pipeline_internal.h"
#include "enhanced/tools/baker/probe_device.h"

namespace openrac::lighting {
namespace {
// Builds the same bounded admission records consumed by the existing protocol coordinator.
json::Value schedule(const pipeline::Options& options, const std::vector<pipeline::Level>& levels) {
    std::string records = "[";

    // Stable IDs preserve numeric export order while admission prioritises large scenes.
    for (std::size_t index = 0; index < levels.size(); ++index) {
        if (index) {
            records += ',';
        }

        records += std::format(
            "{{\"id\":{},\"weight\":{},\"memory\":{}}}",
            index,
            levels[index].bytes,
            levels[index].memory
        );
    }

    return pipeline::parsed(
        std::format(
            "{{\"version\":1,\"threads\":{},\"jobs\":{},\"memory\":{},\"levels\":{}]}}",
            options.threads,
            std::min<std::size_t>(options.jobs, levels.size()),
            options.memory,
            records
        )
    );
}
}  // namespace

int run_bake_pipeline(std::span<const std::string> arguments, std::ostream& output) {
    // Command and filesystem failures retain the launcher error protocol.
    try {
        const profiling::Zone timing("Automatic bake pipeline");
        auto options = pipeline::arguments(arguments);
        const auto levels = pipeline::discover(options);

        // Dry-run validates discovery without changing either exports or the output root.
        if (options.dry_run) {
            // Dry-run lists destinations without creating cache files.
            for (const auto& level : levels) {
                output << level.path.string() << " -> "
                       << (options.output / level.path.filename()).string() << '\n';
            }

            return 0;
        }

        // Adapter discovery precedes worker admission and is shared by every level's cache identity.
        if (options.texture_format == ProbeTextureFormat::Automatic) {
            options.texture_format = detect_probe_texture_format();
        }
        std::filesystem::create_directories(options.output);
        pipeline::CacheLock lock(options.output);
        pipeline::Events events(options, levels, output);
        // Native controllers borrow immutable descriptors until the admission scheduler joins them.
        const auto result = schedule_native_levels(
            schedule(options, levels),
            [&options, &levels, &events](unsigned id, BakeThreadPool& pool) {
                pipeline::run_level(options, levels.at(id), id, events, pool);
            }
        );

        // Invalid admission settings must stop before a partial session can report success.
        if (!result) {
            throw std::runtime_error(result.error().message);
        }

        output << "openrac-output: " << options.output.string() << '\n' << std::flush;
        return events.passed() ? 0 : 1;
    } catch (const std::exception& error) {
        output << "openrac-bake-error: {\"version\":1,\"code\":\"native\",\"title\":\"Lighting "
                  "preparation failed\",\"explanation\":\"The native bake pipeline could not "
                  "complete.\",\"suggestion\":\"Check the bake log and exported inputs, then "
                  "retry.\",\"detail\":"
               << coordinator_string(error.what()) << "}\n"
               << std::flush;
        return 1;
    }
}
}  // namespace openrac::lighting
