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
#include <iomanip>
#include <iterator>
#include <numeric>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string_view>

#include "common/json.h"
#include "common/log.h"
#include "common/profiler.h"
#include "enhanced/lighting/bake_output_queue.h"
#include "enhanced/lighting/probe_density.h"
#include "enhanced/lighting/room_layout.h"
#include "enhanced/lighting/spatial_placement.h"
#include "enhanced/tools/baker/output/batch_output.h"
#include "enhanced/tools/baker/output/debug_preview.h"
#include "enhanced/tools/baker/output/density_stage.h"
#include "enhanced/tools/baker/output/environment_stage.h"
#include "enhanced/tools/baker/output/probe_output.h"
#include "enhanced/tools/baker/output/report.h"
#include "enhanced/tools/baker/pipeline/bake_json.h"
#include "enhanced/tools/baker/pipeline/stage_file.h"
#include "enhanced/tools/baker/probe_rounding.h"
#include "enhanced/tools/baker/sources/activation.h"
#include "enhanced/tools/baker/sources/interior_inputs.h"
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
    std::vector<unsigned> islands{};
};

// One source-of-truth floor partition, plus ownership of the final GI allocations.
void write_islands(
    const fs::path& directory,
    std::span<const EnvironmentFloorSample> floors,
    const bake_detail::SpatialWatershed& field,
    std::span<const BatchJob> jobs
) {
    std::ostringstream out;
    out << std::setprecision(9) << "{\"version\":1,\"method\":\"layered_watershed\",\"samples\":[";
    bool comma = false;
    for (const auto& floor : floors) {
        if (!floor.island) {
            continue;
        }
        out << (comma ? ",[" : "[") << floor.position[0] << ',' << floor.position[1] << ','
            << floor.position[2] << ',' << *floor.island << ',' << floor.island_clearance << ','
            << (floor.swimming_surface && !floor.underwater ? "true" : "false") << ']';
        comma = true;
    }
    out << "],\"islands\":[";
    for (unsigned i = 0; i < field.basins.size(); ++i) {
        const auto& basin = field.basins[i];
        const auto lo = basin.minimum, hi = basin.maximum;
        out << (i ? "," : "") << "{\"island_id\":" << i << ",\"cells\":" << basin.cells
            << ",\"minimum\":[" << lo[0] << ',' << lo[1] << ',' << lo[2] << "],\"maximum\":["
            << hi[0] << ',' << hi[1] << ',' << hi[2]
            << "],\"underwater\":" << (basin.underwater ? "true" : "false")
            << ",\"water_surface\":false}";
    }
    out << "],\"volumes\":{";
    for (unsigned i = 0; i < jobs.size(); ++i) {
        out << (i ? ",\"" : "\"") << jobs[i].path.filename().string() << "\":";
        out << '[';
        for (unsigned j = 0; j < jobs[i].islands.size(); ++j) {
            out << (j ? "," : "") << jobs[i].islands[j];
        }
        out << ']';
    }
    out << "}}\n";
    require_output(write_stage_file(directory / "placement-islands.json", out.str()));
}

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
    bake_detail::RoomLayout& layout,
    std::optional<bake_detail::SpatialPlacement>& spatial,
    const BakeExecution& execution
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

    auto coarse = batch_coarse_grid(exported, sparse, plan, options.settings);

    // Floor-anchor planning replaces automatic boxes when shared inference is available.
    bool anchored_islands = false;
    if (plan["island_policy"]["enabled"].boolean()
        && fs::exists(options.output_directory / "seed-plan.json")) {
        const auto evidence = bake_json::document(options.output_directory / "seed-plan.json");
        const auto& anchors = evidence["floor_anchors"].is_array() ? evidence["floor_anchors"]
                                                                   : evidence["moby_seeds"];
        anchored_islands = !anchors.items().empty();
    }
    if ((!plan["regions"].items().empty() || anchored_islands
         || load_level_bake_profile(options.level_directory).environment.enabled)
        && fs::exists(options.output_directory / "interior-inference.json")
        && fs::exists(options.output_directory / "seed-plan.json")) {
        const auto environment = load_level_bake_profile(options.level_directory).environment;
        floors = read_environment_floors(
            options.output_directory,
            options.level_directory,
            options.settings.traversal,
            environment.require_full_coverage
        );
        // Receiver contacts follow the same gameplay collision as water reachability
        // and the debug picker, independently of decorative render-mesh detail.
        std::optional<BakeScene> collision;
        if (fs::exists(options.level_directory / "collision.glb")) {
            const auto triangles = load_coverage_collision(options.level_directory);
            if (!triangles) {
                throw std::runtime_error(triangles.error().message);
            }
            collision.emplace();
            for (const auto& triangle : *triangles) {
                if (triangle.support) {
                    BakeTriangle solid;
                    solid.vertices = triangle.vertices;
                    collision->triangles.push_back(solid);
                }
            }
        }
        const auto& policy = plan["island_policy"];
        const auto step =
            policy["enabled"].boolean() ? plan_vector<3>(policy["spacing"]) : sparse.spacing;
        const auto save_spatial_debug = [&](const bake_detail::SpatialPlacement& placement) {
            try {
                write_spatial_placement(options.output_directory, placement);
            } catch (const std::exception& error) {
                log::warn("could not save layered watershed debug data: {}", error.what());
            }
        };
        spatial = bake_detail::build_spatial_placement(
            collision ? *collision : exported.scene,
            floors,
            options.settings,
            step,
            unsigned(policy["maximum_probes"].number(4096)),
            unsigned(policy["maximum_regions"].number(1024)),
            environment,
            execution,
            save_spatial_debug
        );
        layout = bake_detail::spatial_floor_layout(spatial->field, floors);
        write_medial_axis_report(options.output_directory, layout, floors);
        save_spatial_debug(*spatial);
    }

    std::vector<BatchJob> jobs{{options.output_directory / "sparse.orpg", coarse}};
    const bool adaptive = density_settings(options.plan).enabled;

    const auto& policy = plan["island_policy"];
    const bool island_grids = policy["enabled"].boolean() && !floors.empty();
    if (island_grids) {
        for (const auto& allocation : spatial->gi.volumes) {
            auto grid = sparse;
            grid.origin = allocation.origin;
            grid.frame = allocation.frame;
            grid.spacing = spatial->gi.settings.spacing;
            grid.dimensions = allocation.dimensions;
            grid.max_distance =
                1.5f
                * std::sqrt(
                    grid.spacing[0] * grid.spacing[0] + grid.spacing[1] * grid.spacing[1]
                    + grid.spacing[2] * grid.spacing[2]
                );
            grid.settings_hash = probe_bake_settings_hash(grid, options.settings);
            grid.validate();
            jobs.push_back(
                {options.output_directory / std::format("region-{:03}.orpg", jobs.size() - 1),
                 grid,
                 {allocation.basin}}
            );
        }
    }

    for (const auto& region : plan["regions"].items()) {
        const bool authored = region["authored"].boolean();
        if (island_grids && !authored) {
            continue;
        }
        auto grid = batch_region_grid(exported, sparse, region);

        if (!floors.empty() && !authored) {
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

    if (island_grids) {
        if (jobs.size() - 1 > static_cast<std::size_t>(policy["maximum_regions"].number())) {
            throw std::runtime_error("watershed GI volumes exceed the configured region limit");
        }
        if (policy["coarse_bounds"].string() == "local_regions" && jobs.size() > 1) {
            Float3 lo{INFINITY, INFINITY, INFINITY}, hi{-INFINITY, -INFINITY, -INFINITY};
            for (unsigned i = 1; i < jobs.size(); ++i) {
                const auto& grid = jobs[i].grid;
                for (unsigned corner = 0; corner < 8; ++corner) {
                    Float3 p{};
                    for (unsigned a = 0; a < 3; ++a) {
                        p[a] = corner & (1u << a) ? grid.spacing[a] * float(grid.dimensions[a] - 1)
                                                  : 0;
                    }
                    p = grid.frame.to_world(p);
                    for (unsigned a = 0; a < 3; ++a) {
                        lo[a] = std::min(lo[a], grid.origin[a] + p[a]);
                        hi[a] = std::max(hi[a], grid.origin[a] + p[a]);
                    }
                }
            }
            for (unsigned a = 0; a < 3; ++a) {
                lo[a] -= float(policy["coarse_padding"].number());
                hi[a] += float(policy["coarse_padding"].number());
            }
            coarse = bounded_probe_grid(exported, lo, hi, sparse.spacing, sparse.map_resolution);
            coarse.game_version = sparse.game_version;
            coarse.level = sparse.level;
            coarse.settings_hash = probe_bake_settings_hash(coarse, options.settings);
            jobs[0].grid = coarse;
        }
    }
    if (!floors.empty()) {
        write_islands(options.output_directory, floors, spatial->field, jobs);
    } else {
        fs::remove(options.output_directory / "placement-islands.json");
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

bool run_probe_batch(
    const scene::LevelData& level,
    const BakeSceneExport& exported,
    const ProbeGrid& sparse_grid,
    const ProbeBatchOptions& options,
    bake_detail::EmissionAverageCache& emission_cache,
    std::ostream& output
) {
    const profiling::Zone trace("Bake level batch");

    std::unique_ptr<BakeThreadPool> local_workers;

    // Coordinated levels borrow one process-wide budget instead of creating competing pools.
    if (!options.pool) {
        local_workers = std::make_unique<BakeThreadPool>(options.threads);
    }

    auto& workers = options.pool ? *options.pool : *local_workers;
    // Both stages read the same immutable export after interior sources have been prepared.
    auto baker = std::make_unique<ProbeBaker>(
        exported.scene, options.settings, options.threads, &workers, &emission_cache
    );
    std::vector<EnvironmentFloorSample> floors;
    bake_detail::RoomLayout layout;
    std::optional<bake_detail::SpatialPlacement> spatial;
    const auto jobs = batch_jobs(
        exported, sparse_grid, options, floors, layout, spatial, {options.threads, &workers}
    );
    auto density = density_settings(options.plan);
    density.texture_format = options.settings.texture_format;
    const auto receivers =
        density.enabled && !floors.empty()
            ? bake_detail::probe_density_receivers(
                  exported.scene, floors, layout, options.settings, options.threads
              )
            : std::vector<bake_detail::ProbeDensityReceiver>{};

    if (probe_input_hash(level.dir, level.overrides) != options.source_snapshot) {
        throw std::runtime_error("source level changed during planning; no batch started");
    }

    const ProbeBakeReport descriptions(level, exported, options.settings, options.light_set);
    std::optional<ProbeVolume> coarse;
    BakeOutputQueue outputs(output);

    // Progress over the whole level, in probes: every grid's, the ones kept counted as done.
    std::size_t total = 0, done = 0, shown = 0;
    std::size_t dense_probes = 0, dense_bytes = 0;
    const auto planned_storage = [&](const ProbeGrid& grid) {
        ProbeVolume volume;
        volume.grid = grid;
        if (grid.map_resolution == 6) {
            volume.colour_format = options.settings.texture_format;
            volume.depth_format = ProbeDepthFormat::Rg16;
        }
        return volume;
    };
    for (std::size_t i = 1; i < jobs.size(); ++i) {
        const auto& grid = jobs[i].grid;
        dense_probes += grid.probe_count();
        const auto planes = planned_storage(grid).plane_sizes();
        dense_bytes += std::accumulate(planes.begin(), planes.end(), std::size_t{0});
    }
    total = jobs.front().grid.probe_count() + dense_probes;
    output << "Planned dense GI: " << jobs.size() - 1 << " grids, " << dense_probes << " probes, "
           << dense_bytes << " atlas bytes\n";
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
                    const auto expected = planned_storage(job.grid);
                    // A format switch must not reuse blocks encoded for another device.
                    if (stored.colour_format != expected.colour_format
                        || stored.depth_format != expected.depth_format) {
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

    const bool environment_unavailable = bake_level_environment(
        {level.dir,
         options.output_directory,
         exported.scene,
         options.settings,
         floors,
         verified_field,
         level.overrides,
         spatial ? &*spatial : nullptr},
        {options.threads, &workers},
        output
    );

    if (probe_input_hash(level.dir, level.overrides) != options.source_snapshot) {
        throw std::runtime_error(
            "source level changed during the bake; no manifest was registered"
        );
    }
    return environment_unavailable;
}

}  // namespace openrac::lighting
