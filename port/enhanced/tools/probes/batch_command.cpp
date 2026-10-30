// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Batch command preparation shared by CLI jobs and memory-admitted coordinator jobs.
#include "enhanced/tools/probes/batch_command.h"

#include <charconv>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <map>
#include <sstream>
#include <stdexcept>

#include "common/json.h"
#include "common/profiler.h"
#include "enhanced/tools/probes/activation.h"
#include "enhanced/tools/probes/automatic_plan.h"
#include "enhanced/tools/probes/batch.h"
#include "enhanced/tools/probes/coverage_stage.h"
#include "enhanced/tools/probes/emission_audit.h"
#include "enhanced/tools/probes/final_plan.h"
#include "enhanced/tools/probes/interior_prepare.h"
#include "enhanced/tools/probes/interior_sources.h"
#include "enhanced/tools/probes/pipeline_internal.h"
#include "enhanced/tools/probes/probe_fingerprint.h"
#include "enhanced/tools/probes/probe_grid.h"
#include "enhanced/tools/probes/report.h"

namespace openrac::lighting {
namespace {
// ---- Arguments ----

// Validated batch-only arguments; transport defaults still come from the tracked level profile.
struct Request {
    std::map<std::string, std::string, std::less<>>
        values;              // CLI overrides, owned by this request.
    bool automatic = false;  // Own final planning without a client request.
    bool resume = false;     // Reuse grids only after their identity and bytes are verified.
};

// Rejects single-grid operations before any scene or generated sidecar is touched.
void parse_request(std::span<const std::string> arguments, Request& request) {
    const std::string_view allowed =
        " --level-dir --batch-output --prepare-interiors --rays --bounces --threads --seed "
        "--resolution --light-set --spacing --environment --sky --sky-intensity --tracy-wait "
        "--profiles-dir ";

    for (std::size_t index = 0; index < arguments.size(); ++index) {
        const auto& key = arguments[index];

        // Automatic mode supplies the final plan directly from the resident scene.
        if (key == "--automatic") {
            request.automatic = true;
            continue;
        }

        // Resume is the batch protocol's only valueless flag.
        if (key == "--resume") {
            request.resume = true;
            continue;
        }

        // Exact token delimiters reject unknown or partial option names.
        if (allowed.find(" " + key + " ") == std::string_view::npos
            || index + 1 == arguments.size()) {
            throw std::runtime_error("invalid or incomplete batch option: " + key);
        }

        request.values[key] = arguments[++index];
    }

    // Both paths are needed before preparation can publish level-owned outputs.
    if (!request.values.contains("--level-dir") || !request.values.contains("--batch-output")) {
        throw std::runtime_error("batch requires --level-dir and --batch-output");
    }
}

// Parses positive-width CLI integer controls without accepting a negative or trailing suffix.
unsigned integer(std::string_view text) {
    unsigned value = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);

    if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) {
        throw std::runtime_error("invalid integer batch option");
    }

    return value;
}

// Parses finite scalar controls without silently accepting trailing text.
float real(const std::string& text) {
    std::size_t end = 0;
    const float value = std::stof(text, &end);

    if (end != text.size() || !std::isfinite(value)) {
        throw std::runtime_error("invalid float batch option");
    }

    return value;
}

// Preserves the standalone batch's optional Tracy connection wait before loading assets.
void wait_for_profiler(const Request& request) {
    if (!request.values.contains("--tracy-wait")) {
        return;
    }

    const float seconds = real(request.values.at("--tracy-wait"));

    // Connection waits are bounded to two minutes, as on the single-grid CLI.
    if (seconds < 0 || seconds > 120) {
        throw std::runtime_error("Tracy wait must be between 0 and 120 seconds");
    }

    if (!profiling::wait_for_connection(
            std::chrono::milliseconds(static_cast<int>(seconds * 1000))
        )) {
        throw std::runtime_error("Tracy capture not connected (requires OPENRAC_TRACY)");
    }
}

// Preserves scalar spacing and explicit anisotropic spacing on the shared batch path.
Float3 spacing(const std::string& text) {
    Float3 result{};

    // A scalar applies equally to each lattice axis.
    if (text.find(',') == std::string::npos) {
        result.fill(real(text));
        return result;
    }

    std::istringstream stream(text);
    std::string component;

    for (float& axis : result) {
        if (!std::getline(stream, component, ',')) {
            throw std::runtime_error("spacing needs X,Y,Z");
        }

        axis = real(component);
    }

    if (std::getline(stream, component, ',') || text.back() == ',') {
        throw std::runtime_error("spacing needs exactly three coordinates");
    }

    return result;
}

// Applies integer transport overrides before the scene and grid identities are calculated.
void integer_overrides(LevelBakeProfile& profile, const Request& request) {
    auto& settings = profile.activation.bake;
    const std::map<std::string, unsigned*, std::less<>> fields{
        {"--rays", &settings.rays_per_probe},
        {"--bounces", &settings.bounces},
        {"--seed", &settings.seed},
        {"--resolution", &profile.map_resolution},
        {"--light-set", &profile.activation.light_set}
    };

    for (const auto& [key, target] : fields) {
        if (request.values.contains(key)) {
            *target = integer(request.values.at(key));
        }
    }
}

// Resolves the same scalar environment and grid overrides used by single-level command lines.
LevelBakeProfile resolved_profile(const Request& request) {
    auto profile = load_level_bake_profile(request.values.at("--level-dir"));
    integer_overrides(profile, request);

    for (const auto& [key, value] : request.values) {
        if (key == "--spacing") {
            profile.sparse_spacing = spacing(value);
        } else if (key == "--environment") {
            profile.activation.environment.fill(real(value));
        } else if (key == "--sky-intensity") {
            profile.activation.bake.sky_intensity = real(value);

            if (profile.activation.bake.sky_intensity < 0) {
                throw std::runtime_error("negative sky intensity");
            }
        } else if (key == "--sky") {
            if (value != "level" && value != "uniform") {
                throw std::runtime_error("sky must be level or uniform");
            }

            profile.activation.bake.use_level_sky = value == "level";
        }
    }

    return profile;
}

// ---- Scene preparation ----

// Publishes the same generated source report before the dependent grid plan is requested.
void prepare_sources(
    const scene::LevelData& level,
    BakeSceneExport& exported,
    const ProbeBatchOptions& options,
    std::ostream& output
) {
    const profiling::Zone zone("Prepare interior sources");
    const auto report = apply_interior_sources(level, exported.scene);

    if (!report) {
        throw std::runtime_error(report.error().message);
    }

    if (report->empty()) {
        return;
    }

    output << "interior_area_emitters " << exported.scene.area_emitters.size() << '\n';
    const auto path = options.output_directory / "interior-lights-report.json";

    if (options.resume) {
        std::filesystem::remove(path);
        std::filesystem::remove(path.string() + ".tmp");
    }

    write_bake_file(
        path, std::span(reinterpret_cast<const std::uint8_t*>(report->data()), report->size())
    );
}

// Loads and audits the level after the scheduler has admitted its estimated memory footprint.
scene::LevelData load_level(
    const std::filesystem::path& directory,
    const std::filesystem::path& output_directory,
    std::ostream& output
) {
    const profiling::Zone zone("Load bake assets");
    scene::LevelData level;
    std::string error;
    output << "Loading extracted level " << directory.string() << '\n' << std::flush;

    if (!scene::load_level(directory, level, error)) {
        throw std::runtime_error(error);
    }

    if (level.level < 0) {
        throw std::runtime_error("expected an identified RAC1 PAL or NTSC export");
    }

    const auto audit = audit_level_emission(level, output_directory);

    // Audit publication must finish before grids can be considered verified.
    if (!audit) {
        throw std::runtime_error(audit.error().message);
    }

    output << "Emission audit: " << *audit
           << " undefined full-bright material candidates (not automatically emitted)\n";

    return level;
}

// Retains the original PAL/NTSC identity checks before transport sees an exported scene.
std::string game_version(const std::filesystem::path& directory) {
    json::Value manifest;
    std::string error;

    if (!json::parse_file((directory / "manifest.json").string(), manifest, error)) {
        throw std::runtime_error(error);
    }

    const auto& version = manifest["version"].string();

    if (manifest["game"].string() != "rac1" || (version != "pal" && version != "ntsc")) {
        throw std::runtime_error("expected a RAC1 PAL or NTSC export");
    }

    return "rac1-" + version;
}

// ---- Batch execution ----

// Prepares generated interiors before the immutable transport source snapshot is captured.
void prepare_interiors(
    const Request& request, const ProbeBatchOptions& options, std::ostream& output
) {
    // Explicit generation finishes before scene hashing and loading.
    if (request.values.contains("--prepare-interiors")) {
        output << "Bake stage: Infer interior lighting\n" << std::flush;
        const auto result = prepare_level_interiors(
            options.level_directory,
            request.values.at("--prepare-interiors"),
            options.output_directory,
            !request.automatic
                || pipeline::region_settings(options.level_directory)["interior_lighting"].boolean()
        );

        // A missing interior report cannot become part of a verified lighting scene.
        if (!result) {
            throw std::runtime_error(result.error().message);
        }
    }
}

// Captures generated source identity before transport and makes interrupted first bakes resumable.
void prepare_batch_inputs(
    const Request& request, ProbeBatchOptions& options, std::ostream& output
) {
    prepare_interiors(request, options, output);

    options.source_snapshot = probe_input_hash(options.level_directory);

    // Generated interior sources must not discard completed grids after an interrupted first bake.
    if (request.automatic) {
        pipeline::update_partial_sources(options.output_directory, options.source_snapshot);
    }
}

// Borrowed protocol streams, unused by the direct native automatic path.
struct PlanStreams {
    std::istream& input;   // Caller-owned input, alive through planning.
    std::ostream& output;  // Caller-owned diagnostics, alive through the batch.
};

// Keeps automatic plan ownership native while retaining explicit reference-client requests.
Result<std::string> resolve_plan(
    const Request& request,
    ProbeBatchOptions& options,
    const LevelBakeProfile& profile,
    const std::string& inspection,
    PlanStreams streams
) {
    // Inspection clients keep their bounded request protocol.
    if (!request.automatic) {
        return read_covered_plan(options, streams.input, streams.output);
    }

    const auto profiles = request.values.contains("--profiles-dir")
                              ? std::filesystem::path(request.values.at("--profiles-dir"))
                              : std::filesystem::path("port/enhanced/lighting/profiles");
    return automatic_plan(options, profile, inspection, profiles);
}

// Keeps one scene alive through native planning, transport and verified output registration.
void execute(
    const Request& request, std::istream& input, std::ostream& output, BakeThreadPool* pool
) {
    const auto profile = resolved_profile(request);
    ProbeBatchOptions options;
    options.level_directory = request.values.at("--level-dir");
    options.output_directory = request.values.at("--batch-output");
    options.settings = profile.activation.bake;
    options.light_set = profile.activation.light_set;
    options.threads =
        request.values.contains("--threads") ? integer(request.values.at("--threads")) : 0;
    options.pool = pool;
    options.resume = request.resume;
    const auto identity = game_version(options.level_directory);

    prepare_batch_inputs(request, options, output);
    auto level = load_level(options.level_directory, options.output_directory, output);
    auto exported = export_probe_scene(level, options.light_set, profile.activation.environment);
    prepare_sources(level, exported, options, output);
    auto grid = sparse_probe_grid(exported, profile.sparse_spacing, profile.map_resolution);
    grid.game_version = identity;
    grid.level = static_cast<std::uint32_t>(level.level);
    grid.settings_hash = probe_bake_settings_hash(grid, options.settings);
    const auto inspection =
        describe_probe_bake(level, exported, grid, options.settings, options.light_set);
    output << inspection;
    options.scene_hash = exported.scene_hash;
    output << "Probe batch ready\n" << std::flush;

    {
        const profiling::Zone zone("Prepare region plan");

        const auto plan = resolve_plan(request, options, profile, inspection, {input, output});

        // Invalid automatic metadata cannot reach transport or registration.
        if (!plan) {
            throw std::runtime_error(plan.error().message);
        }

        options.plan = *plan;
        options.register_outputs = options.register_outputs || request.automatic;
    }

    // An automatic run registers density results only if this run produced them.
    if (options.register_outputs) {
        std::filesystem::remove(options.output_directory / "probe-density-report.json");
    }

    run_probe_batch(level, exported, grid, options, output);

    // Reference batch requests omit automatic registration but share all transport verification.
    if (options.register_outputs) {
        const auto result = register_batch_outputs(options);

        // Completion must follow successful registration, never merely successful transport.
        if (!result) {
            throw std::runtime_error(result.error().message);
        }
    }

    output << "Probe batch complete\n" << std::flush;
}
}  // namespace

int run_batch_command(
    std::span<const std::string> arguments,
    std::istream& input,
    std::ostream& output,
    BakeThreadPool* pool
) {
    try {
        const profiling::Zone zone("Level bake pipeline");
        Request request;
        parse_request(arguments, request);
        wait_for_profiler(request);
        execute(request, input, output, pool);
        return 0;
    } catch (const NothingToBake& error) {
        output << "probe bake: " << error.what() << '\n' << std::flush;
        return kNothingToBake;
    } catch (const std::exception& error) {
        output << "probe bake: " << error.what() << '\n' << std::flush;
        return 1;
    } catch (...) {
        output << "probe bake: unknown native batch failure\n" << std::flush;
        return 1;
    }
}
}  // namespace openrac::lighting
