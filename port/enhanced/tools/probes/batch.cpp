// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Persistent single-level baking and shared CLI diagnostics/output.
#include "enhanced/tools/probes/batch.h"

#include <algorithm>
#include <cfenv>
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
#include "enhanced/lighting/probe_density.h"
#include "enhanced/lighting/room_layout.h"
#include "enhanced/tools/probes/density_stage.h"
#include "enhanced/tools/probes/environment_stage.h"
#include "enhanced/tools/probes/probe_fingerprint.h"
#include "enhanced/tools/probes/probe_grid.h"
#include "enhanced/tools/probes/report.h"

namespace fs = std::filesystem;

namespace openrac::lighting {
namespace {
// Restore the caller's arithmetic mode even if read-back diagnostics reject a volume.
class RuntimeRounding {
public:
    RuntimeRounding() {
        if (std::fegetenv(&m_previous) != 0 || std::fesetround(FE_TOWARDZERO) != 0) {
            throw std::runtime_error("cannot set runtime probe verification rounding");
        }
    }

    ~RuntimeRounding() { std::fesetenv(&m_previous); }

    RuntimeRounding(const RuntimeRounding&) = delete;
    RuntimeRounding& operator=(const RuntimeRounding&) = delete;

private:
    std::fenv_t m_previous{};  // restored before the next grid is traced
};

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
    json::Value plan;
    std::string error;
    constexpr std::size_t kMaximumRegions = 1024;  // runtime dense-region limit

    if (!json::parse(options.plan, plan, error) || plan["version"].number() != 1
        || plan["scene_hash"].string() != std::to_string(exported.scene_hash)
        || !plan["regions"].is_array() || plan["regions"].items().size() > kMaximumRegions) {
        throw std::runtime_error("invalid or stale probe batch plan: " + error);
    }

    const auto coarse = batch_coarse_grid(exported, sparse, plan, options.settings);

    if (fs::exists(options.output_directory / "interior-inference.json")
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
    std::ifstream input(job.path, std::ios::binary | std::ios::ate);

    if (!input || input.tellg() != static_cast<std::streamoff>(encoded.size())) {
        throw std::runtime_error("probe batch read-back size mismatch");
    }

    std::vector<std::uint8_t> bytes(encoded.size());
    input.seekg(0);

    if (!input.read(
            reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())
        )
        || bytes != encoded) {
        throw std::runtime_error("probe batch read-back bytes differ");
    }

    auto volume = decode_probes(bytes);
    const auto& grid = job.grid;

    if (!probe_identity_matches(
            volume, grid.game_version, grid.level, grid.scene_hash, grid.settings_hash
        )) {
        throw std::runtime_error("probe batch identity mismatch");
    }

    return volume;
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
    const auto density = density_settings(options.plan);
    const auto receivers =
        density.enabled && !floors.empty()
            ? bake_detail::probe_density_receivers(
                  exported.scene, floors, layout, options.settings, options.threads
              )
            : std::vector<bake_detail::ProbeDensityReceiver>{};

    if (probe_input_hash(level.dir) != options.source_snapshot) {
        throw std::runtime_error("source level changed during planning; no batch started");
    }

    auto baker = std::make_unique<ProbeBaker>(exported.scene, options.settings, options.threads);
    const ProbeBakeReport descriptions(level, exported, options.settings, options.light_set);
    std::optional<ProbeVolume> coarse;

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
                try {
                    RuntimeRounding rounding;

                    if (density.enabled && coarse) {
                        std::ifstream report_file(job.path.string() + ".txt");
                        const std::string report((std::istreambuf_iterator<char>(report_file)), {});

                        if (report.find(identity) == std::string::npos) {
                            throw std::runtime_error("kept probe grid has another density policy");
                        }

                        final_job.grid = decode_probes(*bytes).grid;
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
        bake_detail::ProbeDensityReport density_report;

        if (coarse && density.enabled) {
            const auto fine_bytes = encode_probes(volume);
            const auto fine_report = descriptions.describe(job.grid) + identity
                                     + std::format(
                                         "adaptive_original_probes {}\nadaptive_final_probes {}\n"
                                         "adaptive_overlap_restored 1\n",
                                         volume.probes.size(),
                                         volume.probes.size()
                                     );
            volume = bake_detail::simplify_probe_density(
                std::move(volume), *coarse, receivers, density, density_report
            );
            volume.grid.settings_hash = probe_bake_settings_hash(volume.grid, options.settings);
            final_job.grid = volume.grid;
            const auto reference =
                job.path.parent_path() / "adaptive-reference" / job.path.filename();
            fs::remove(reference);
            fs::remove(reference.string() + ".txt");

            if (volume.probes.size() < job.grid.probe_count()) {
                fs::create_directories(reference.parent_path());
                write_bake_file(reference, fine_bytes);
                write_bake_file(
                    reference.string() + ".txt",
                    std::span(
                        reinterpret_cast<const std::uint8_t*>(fine_report.data()),
                        fine_report.size()
                    )
                );
            }

            output << "Adaptive " << job.path.filename().string() << ' '
                   << density_report.original_probes << " -> " << density_report.final_probes
                   << " probes, " << density_report.receivers << " receivers\n"
                   << std::flush;
        }

        const auto encoded = encode_probes(volume);
        write_bake_file(job.path, encoded);
        {
            RuntimeRounding rounding;
            auto checked = verify_batch_file(final_job, encoded);

            if (checked.grid.settings_hash
                != probe_bake_settings_hash(checked.grid, options.settings)) {
                throw std::runtime_error("probe batch settings differ under runtime rounding");
            }

            if (coarse) {
                validate_probe_pair(*coarse, checked);
            }

            report_terrain_coverage(level, checked, coarse ? &*coarse : nullptr, output);

            if (!coarse) {
                coarse = std::move(checked);
            }
        }
        auto report = descriptions.describe(final_job.grid);
        report += identity;

        if (coarse && density.enabled && density_report.original_probes) {
            report += std::format(
                "adaptive_original_probes {}\nadaptive_final_probes {}\nadaptive_receivers {}\n"
                "adaptive_comparisons {}\nadaptive_accepted_axes {}\nadaptive_rejected_axes {}\n"
                "adaptive_maximum_error {}\n",
                density_report.original_probes,
                density_report.final_probes,
                density_report.receivers,
                density_report.comparisons,
                density_report.accepted_axes,
                density_report.rejected_axes,
                density_report.maximum_error
            );
        }

        const auto valid =
            std::count_if(volume.probes.begin(), volume.probes.end(), [](const Probe& probe) {
                return probe.valid;
            });
        const auto relocated =
            std::count_if(volume.probes.begin(), volume.probes.end(), [](const Probe& probe) {
                return probe.relocated;
            });
        report += std::format(
            "relocated_probes {}\nvalid_probes {}\ninvalid_probes {}\n"
            "bake_seconds {}\nasset_bytes {}\nverified_runtime_rounding 1\n",
            relocated,
            valid,
            volume.probes.size() - static_cast<std::size_t>(valid),
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count(),
            encoded.size()
        );
        write_bake_file(
            job.path.string() + ".txt",
            std::span(reinterpret_cast<const std::uint8_t*>(report.data()), report.size())
        );
        output << "Verified " << job.path.filename().string() << '\n' << std::flush;
    }

    // Release GI acceleration before constructing reflection capture state.
    baker.reset();

    if (density.enabled && !receivers.empty()) {
        std::vector<fs::path> paths;

        for (const auto& job : jobs) {
            paths.push_back(job.path);
        }

        verify_probe_density_field(paths, receivers, density, output);
    }

    // A resumed stage regenerates captures only after every GI grid has passed read-back checks.
    if (options.resume) {
        std::filesystem::remove(options.output_directory / "environment.orpe");
        std::filesystem::remove(options.output_directory / "environment.orpe.tmp");
    }

    bake_level_environment(
        level.dir,
        options.output_directory,
        exported.scene,
        options.settings,
        options.threads,
        output
    );

    if (probe_input_hash(level.dir) != options.source_snapshot) {
        throw std::runtime_error(
            "source level changed during the bake; no manifest was registered"
        );
    }

    output << "Probe batch complete\n" << std::flush;
}

}  // namespace openrac::lighting
