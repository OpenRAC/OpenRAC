// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Persistent single-level baking and shared CLI diagnostics/output.
#include "enhanced/tools/baker/batch.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string_view>

#include "common/json.h"
#include "common/profiler.h"
#include "enhanced/lighting/bake_output_queue.h"
#include "enhanced/lighting/probe_density.h"
#include "enhanced/lighting/room_layout.h"
#include "enhanced/tools/baker/output/batch_output.h"
#include "enhanced/tools/baker/output/density_stage.h"
#include "enhanced/tools/baker/output/environment_stage.h"
#include "enhanced/tools/baker/output/probe_output.h"
#include "enhanced/tools/baker/output/report.h"
#include "enhanced/tools/baker/probe_rounding.h"
#include "enhanced/tools/baker/sources/probe_fingerprint.h"
#include "enhanced/tools/baker/sources/probe_grid.h"

namespace fs = std::filesystem;

namespace openrac::lighting {
namespace {
// Stops the controller on output failure; the queue still owns and drains every later task.
void require_output(Result<void> result) {
    if (!result) {
        throw std::runtime_error(result.error().message);
    }
}

// Fixed names prevent planner input from redirecting writes to unrelated assets.
struct BatchJob {
    fs::path path;
    ProbeGrid grid;
};

// Allocation policy is level-local and participates in resume through the report identity.
bake_detail::ProbeDensitySettings density_settings(const std::string& text) {
    json::Value plan;
    std::string error;

    if (!json::parse(text, plan, error)) {
        throw std::runtime_error("invalid probe density plan: " + error);
    }

    const auto& value = plan["adaptive_density"];
    bake_detail::ProbeDensitySettings settings;

    if (!value.is_null()) {
        if (value["enabled"].type() != json::Value::Type::Bool
            || !value["relative_error"].is_number() || !value["absolute_error"].is_number()) {
            throw std::runtime_error("invalid adaptive probe settings");
        }

        settings.enabled = value["enabled"].boolean();
        settings.relative_error = float(value["relative_error"].number());
        settings.absolute_error = float(value["absolute_error"].number());
    }

    settings.validate();
    return settings;
}

// A resumed reduced grid is reusable only for this exact fine grid and validation policy.
std::string density_identity(
    const ProbeGrid& grid, const bake_detail::ProbeDensitySettings& settings
) {
    return std::format(
        "adaptive_source_hash {}\nadaptive_policy 2 {} {} {}\n",
        grid.settings_hash,
        settings.enabled,
        settings.relative_error,
        settings.absolute_error
    );
}

// Reject malformed vector settings before any volume output is written.
template <std::size_t N>
std::array<float, N> plan_vector(const json::Value& value) {
    if (!value.is_array() || value.items().size() != N) {
        throw std::runtime_error("invalid probe batch vector");
    }

    std::array<float, N> result{};

    for (std::size_t i = 0; i < N; ++i) {
        if (!value[i].is_number() || !std::isfinite(value[i].number())) {
            throw std::runtime_error("nonfinite probe batch vector");
        }

        result[i] = static_cast<float>(value[i].number());
    }

    return result;
}

// Keeps exact repartitioned cell endpoints; ordinary authored plans still use world bounds.
ProbeGrid batch_region_grid(
    const BakeSceneExport& exported, const ProbeGrid& sparse, const json::Value& region
) {
    const auto spacing = plan_vector<3>(region["spacing"]);

    // Repartitioned grids already have integer coverage, so float ceil must not expand them again.
    if (region["lattice_bounds"].is_array()) {
        return lattice_probe_grid(
            exported, plan_vector<6>(region["lattice_bounds"]), spacing, sparse.map_resolution
        );
    }

    const auto bounds = plan_vector<6>(region["bounds"]);
    return bounded_probe_grid(
        exported,
        {bounds[0], bounds[1], bounds[2]},
        {bounds[3], bounds[4], bounds[5]},
        spacing,
        sparse.map_resolution
    );
}

// Optional allocation bounds leave transport geometry and cell-local visibility ranges unchanged.
ProbeGrid batch_coarse_grid(
    const BakeSceneExport& exported,
    const ProbeGrid& sparse,
    const json::Value& plan,
    const BakeSettings& settings
) {
    auto coarse = sparse;

    // Allocation can follow gameplay coverage while transport still sees the complete scene.
    if (!plan["coarse_bounds"].is_null()) {
        const auto bounds = plan_vector<6>(plan["coarse_bounds"]);
        coarse = bounded_probe_grid(
            exported,
            {bounds[0], bounds[1], bounds[2]},
            {bounds[3], bounds[4], bounds[5]},
            sparse.spacing,
            sparse.map_resolution
        );
        coarse.game_version = sparse.game_version;
        coarse.level = sparse.level;
        coarse.max_distance = sparse.max_distance;
        coarse.settings_hash = probe_bake_settings_hash(coarse, settings);
    }

    return coarse;
}

// Parse all grids first so bad metadata or existing outputs cannot produce a partial fresh bake.
std::vector<BatchJob> batch_jobs(
    const BakeSceneExport& exported,
    const ProbeGrid& sparse,
    const ProbeBatchOptions& options,
    std::vector<EnvironmentFloorSample>& floors,
    bake_detail::RoomLayout& layout
) {
    const profiling::Zone timing("Prepare batch grids");

    json::Value plan;
    std::string error;
    constexpr std::size_t kMaximumRegions = 1024;  // runtime dense-region limit

    if (!json::parse(options.plan, plan, error) || plan["version"].number() != 1
        || plan["scene_hash"].string() != std::to_string(exported.scene_hash)
        || !plan["regions"].is_array() || plan["regions"].items().size() > kMaximumRegions) {
        throw std::runtime_error("invalid or stale probe batch plan: " + error);
    }

    const auto coarse = batch_coarse_grid(exported, sparse, plan, options.settings);

    // Sparse-only batches do not fit local grids; reflection placement loads its own floor graph.
    if (!plan["regions"].items().empty()
        && fs::exists(options.output_directory / "interior-inference.json")
        && fs::exists(options.output_directory / "seed-plan.json")) {
        floors = read_environment_floors(
            options.output_directory, options.level_directory, options.settings.traversal
        );
        layout = bake_detail::prepare_room_layout(
            exported.scene, floors, options.settings, options.threads
        );
    }

    if (!floors.empty()) {
        write_medial_axis_report(options.output_directory, layout, floors);
    }

    std::vector<BatchJob> jobs{{options.output_directory / "sparse.orpg", coarse}};
    const bool adaptive = density_settings(options.plan).enabled;

    for (const auto& region : plan["regions"].items()) {
        auto grid = batch_region_grid(exported, sparse, region);

        if (!floors.empty()) {
            grid = bake_detail::fit_room_grid(grid, exported.scene, floors, layout, adaptive);
        }

        if (!region["max_distance"].is_number()) {
            throw std::runtime_error("probe batch moment distance missing");
        }

        grid.max_distance = static_cast<float>(region["max_distance"].number());
        grid.game_version = sparse.game_version;
        grid.level = sparse.level;
        grid.settings_hash = probe_bake_settings_hash(grid, options.settings);
        grid.validate();
        jobs.push_back(
            {options.output_directory / std::format("region-{:03}.orpg", jobs.size() - 1), grid}
        );
    }

    for (const auto& job : jobs) {
        for (const auto& suffix : {"", ".txt", ".tmp", ".txt.tmp"}) {
            const auto path = job.path.string() + suffix;

            if (!fs::exists(path)) {
                continue;
            }

            // Resuming: a write that was cut off is thrown away; a finished grid is checked later.
            if (options.resume && std::string_view(suffix).ends_with(".tmp")) {
                fs::remove(path);
            } else if (!options.resume) {
                throw std::runtime_error("probe batch output already exists");
            }
        }
    }

    return jobs;
}

// A grid a stopped bake already wrote, with its report: its bytes, or nothing when either file
// is missing.
std::optional<std::vector<std::uint8_t>> finished_bytes(const BatchJob& job) {
    const profiling::Zone timing("Read cached probe asset");

    if (!fs::exists(job.path) || !fs::exists(job.path.string() + ".txt")) {
        return std::nullopt;
    }

    std::ifstream input(job.path, std::ios::binary);
    std::vector<std::uint8_t> bytes(
        (std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>()
    );

    if (!input.good() && !input.eof()) {
        return std::nullopt;
    }

    return bytes;
}

// Read the actual file back and verify exactly the bytes produced by this grid.
ProbeVolume verify_batch_file(const BatchJob& job, const std::vector<std::uint8_t>& encoded) {
    auto checked = verify_probe_file(job.path, job.grid, encoded);

    if (!checked) {
        throw std::runtime_error(checked.error().message);
    }

    return std::move(*checked);
}
}  // namespace

void run_probe_batch(
    const scene::LevelData& level,
    const BakeSceneExport& exported,
    const ProbeGrid& sparse_grid,
    const ProbeBatchOptions& options,
    std::ostream& output
) {
    const profiling::Zone trace("Bake level batch");

    std::vector<EnvironmentFloorSample> floors;
    bake_detail::RoomLayout layout;
    const auto jobs = batch_jobs(exported, sparse_grid, options, floors, layout);
    auto density = density_settings(options.plan);
    density.texture_format = options.settings.texture_format;
    const auto receivers =
        density.enabled && !floors.empty()
            ? bake_detail::probe_density_receivers(
                  exported.scene, floors, layout, options.settings, options.threads
              )
            : std::vector<bake_detail::ProbeDensityReceiver>{};

    if (probe_input_hash(level.dir) != options.source_snapshot) {
        throw std::runtime_error("source level changed during planning; no batch started");
    }

    std::unique_ptr<BakeThreadPool> local_workers;

    // Coordinated levels borrow one process-wide budget instead of creating competing pools.
    if (!options.pool) {
        local_workers = std::make_unique<BakeThreadPool>(options.threads);
    }

    auto& workers = options.pool ? *options.pool : *local_workers;
    auto baker =
        std::make_unique<ProbeBaker>(exported.scene, options.settings, options.threads, &workers);
    const ProbeBakeReport descriptions(level, exported, options.settings, options.light_set);
    std::optional<ProbeVolume> coarse;
    BakeOutputQueue outputs(output);

    // Progress over the whole level, in probes: every grid's, the ones kept counted as done.
    std::size_t total = 0, done = 0, shown = 0;

    for (const auto& job : jobs) {
        total += job.grid.probe_count();
    }

    output << "Probe batch probes " << total << '\n' << std::flush;
    const auto report_done = [&](std::size_t now) {
        // Each half per cent at most: a level has up to a few million probes.
        if (total > 0 && (now == total || (now - shown) * 200 >= total)) {
            output << "Probes " << now << '/' << total << '\n' << std::flush;
            shown = now;
        }
    };

    for (const auto& job : jobs) {
        require_output(outputs.make_room());
#ifdef OPENRAC_TRACY
        // Formatting the dynamic label should cost nothing in an ordinary bake.
        const profiling::Zone grid_trace(job.path.filename().string());
#endif
        profiling::frame("Probe volumes");
        const auto start = std::chrono::steady_clock::now();
        const auto identity = density_identity(job.grid, density);
        auto final_job = job;

        if (options.resume) {
            if (const auto bytes = finished_bytes(job)) {
                // Cached validation stays ordered without serialising newly baked outputs.
                require_output(outputs.finish());
                try {
                    RuntimeProbeRounding rounding;
                    const auto stored = decode_gpu_probes(*bytes);
                    const auto expected_format = job.grid.map_resolution == 6
                                                     ? options.settings.texture_format
                                                     : ProbeTextureFormat::R11G11B10;
                    // A format switch must not reuse blocks encoded for another device.
                    if (stored.colour_format != expected_format) {
                        throw std::runtime_error("kept probe grid has another compression format");
                    }

                    if (density.enabled && coarse) {
                        std::ifstream report_file(job.path.string() + ".txt");
                        const std::string report((std::istreambuf_iterator<char>(report_file)), {});

                        if (report.find(identity) == std::string::npos) {
                            throw std::runtime_error("kept probe grid has another density policy");
                        }

                        // Only packed validation is needed to recover a cached adaptive grid header.
                        final_job.grid = stored.grid;
                    }

                    auto checked = verify_batch_file(final_job, *bytes);

                    if (checked.grid.settings_hash
                        != probe_bake_settings_hash(checked.grid, options.settings)) {
                        throw std::runtime_error("kept probe grid has other settings");
                    }

                    if (coarse) {
                        validate_probe_pair(*coarse, checked);
                    } else {
                        coarse = std::move(checked);
                    }

                    output << "Kept " << job.path.filename().string() << '\n'
                           << "Verified " << job.path.filename().string() << '\n'
                           << std::flush;
                    done += job.grid.probe_count();
                    report_done(done);
                    continue;
                } catch (const std::exception& error) {
                    output << "Lighting warning: rebuilding rejected cached grid "
                           << job.path.filename().string() << ": " << error.what() << '\n'
                           << std::flush;
                }
            }

            // What is left of a grid the stop cut short (written, its report not yet): baked again.
            fs::remove(job.path);
            fs::remove(job.path.string() + ".txt");
        }

        output << "Baking " << job.path.filename().string() << '\n' << std::flush;
        auto volume = baker->bake(
            job.grid,
            [&](std::size_t baked, std::size_t) { report_done(done + baked); },
            options.threads
        );
        done += job.grid.probe_count();
        report_done(done);
        auto owned = std::make_shared<ProbeGridOutput>(
            ProbeGridOutput{job.path, std::move(volume), start, identity}
        );
        const ProbeGridOutputContext context{
            options,
            descriptions,
            receivers,
            density,
            {level, options.settings, coarse ? &*coarse : nullptr},
            workers
        };

        // Every dependent grid must see a fully verified coarse field.
        if (!coarse) {
            auto completed = prepare_probe_grid_output(*owned, context);

            if (!completed) {
                throw std::runtime_error(completed.error().message);
            }

            coarse = std::move(completed->coarse);
            output << completed->diagnostics << std::flush;
        } else {
            // Joined output controllers own raw maps while submitting compute to the same shared pool.
            require_output(outputs.submit([owned, context] {
                auto completed = prepare_probe_grid_output(*owned, context);

                if (!completed) {
                    throw std::runtime_error(completed.error().message);
                }

                return std::move(completed->diagnostics);
            }));
        }
    }

    require_output(outputs.finish());

    // Release GI acceleration before constructing reflection capture state.
    baker.reset();

    std::vector<ProbeVolume> verified_field;

    if (density.enabled && !receivers.empty()) {
        std::vector<fs::path> paths;

        for (const auto& job : jobs) {
            paths.push_back(job.path);
        }

        verified_field = verify_probe_density_field(
            paths, receivers, density, output, {options.threads, &workers}
        );
    }

    // A resumed stage regenerates captures only after every GI grid has passed read-back checks.
    if (options.resume) {
        std::filesystem::remove(options.output_directory / "environment.orpe");
        std::filesystem::remove(options.output_directory / "environment.orpe.tmp");
    }

    bake_level_environment(
        {level.dir,
         options.output_directory,
         exported.scene,
         options.settings,
         floors,
         verified_field},
        {options.threads, &workers},
        output
    );

    if (probe_input_hash(level.dir) != options.source_snapshot) {
        throw std::runtime_error(
            "source level changed during the bake; no manifest was registered"
        );
    }
}

}  // namespace openrac::lighting
