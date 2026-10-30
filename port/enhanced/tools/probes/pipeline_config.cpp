// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Automatic pipeline configuration and complete export discovery before native admission.
#include <algorithm>
#include <charconv>
#include <cmath>
#include <map>
#include <regex>
#include <stdexcept>
#include <thread>

#include "enhanced/tools/probes/bake_json.h"
#include "enhanced/tools/probes/coordinator_protocol.h"
#include "enhanced/tools/probes/pipeline_internal.h"

namespace openrac::lighting::pipeline {
namespace {
// ---- Command controls ----

// Parses bounded whole-number CLI values without accepting trailing text or negative values.
unsigned integer(const std::string& text) {
    unsigned result = 0;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result);

    // A malformed CPU or quality budget cannot become an accidental zero/default.
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size()) {
        throw std::runtime_error("invalid integer pipeline option: " + text);
    }

    return result;
}

// Detects a canonical descendant by components, including symlink-resolved export roots.
bool within(const std::filesystem::path& path, const std::filesystem::path& root) {
    const auto relative = path.lexically_relative(root);
    return path == root || (!relative.empty() && *relative.begin() != "..");
}

// Resolves path-valued flags without mixing them with integer budget parsing.
bool path_option(Options& options, const std::string& key, const std::string& value) {
    const std::map<std::string, std::filesystem::path*> paths{
        {"--levels-dir", &options.levels},
        {"--output-dir", &options.output},
        {"--profiles-dir", &options.profiles}
    };
    const auto found = paths.find(key);

    // Non-path flags are validated separately as numeric controls.
    if (found == paths.end()) {
        return false;
    }

    *found->second = value;
    return true;
}

// Applies a single key/value pair, rejecting unsupported flags before writing cache files.
void option(Options& options, const std::string& key, const std::string& value) {
    // Path values bypass numeric quality and CPU parsing.
    if (path_option(options, key, value)) {
        return;
    }

    // Only declared budget and quality flags may reach numeric parsing.
    if (key == "--jobs") {
        options.jobs = integer(value);
    } else if (key == "--threads") {
        options.threads = integer(value);

        // An explicit zero requests all logical CPUs rather than the background default.
        if (!options.threads) {
            options.threads = std::min(256u, std::max(1u, std::thread::hardware_concurrency()));
        }
    } else if (key == "--memory-mib") {
        options.memory = std::uint64_t{integer(value)} * 1024 * 1024;
    } else if (key == "--rays") {
        options.rays = integer(value);

        // Zero rays cannot silently select a different lighting quality.
        if (!options.rays) {
            throw std::runtime_error("rays must be positive");
        }
    } else if (key == "--bounces") {
        const auto bounces = integer(value);

        // The transport override must fit the supported bounce budget before narrowing.
        if (bounces > 64) {
            throw std::runtime_error("bounce limit exceeds 64");
        }

        options.bounces = static_cast<int>(bounces);
    } else {
        throw std::runtime_error("unknown pipeline option: " + key);
    }
}

// Rejects region controls that Python previously validated before sending its final request.
void validate_regions(const json::Value& settings, const json::Value& defaults) {
    // Every supplied field has a known default and a fixed scalar type.
    for (const auto& [key, value] : settings.members()) {
        const auto& expected = defaults[key];

        // Unknown or mistyped controls cannot silently alter default coverage.
        if (expected.is_null() || value.type() != expected.type()
            || (value.is_number() && (!std::isfinite(value.number()) || value.number() < 0))) {
            throw std::runtime_error("invalid region planning setting: " + key);
        }
    }

    const auto& policy = settings["coarse_bounds"].string();

    // Coarse bounds have only scene-wide and local-coverage modes.
    if (policy != "scene" && policy != "local_regions") {
        throw std::runtime_error("coarse_bounds must be scene or local_regions");
    }

    const auto probes = coordinator_integer(settings["max_probes"], 1'000'000);
    const auto regions = coordinator_integer(settings["max_regions"], 1024);

    // Bounds match native planning and adaptive validation budgets.
    if (probes < 64 || !regions || settings["geometry_max_rise"].number() <= 0
        || settings["adaptive_relative_error"].number() > .25
        || settings["adaptive_absolute_error"].number() > .1) {
        throw std::runtime_error("region planning limits exceed supported budgets");
    }
}

// Resolves and validates filesystem boundaries before cache creation or admission.
void resolve_paths(Options& options) {
    // Both roots are mandatory; canonicalization resolves symlinks before recursive operations.
    if (options.levels.empty() || options.output.empty()) {
        throw std::runtime_error("--bake-all requires --levels-dir and --output-dir");
    }

    options.levels = std::filesystem::canonical(options.levels);
    options.output = std::filesystem::weakly_canonical(std::filesystem::absolute(options.output));
    options.profiles = std::filesystem::absolute(options.profiles);

    // Outputs may not recursively include exports or be nested inside them.
    if (within(options.output, options.levels) || within(options.levels, options.output)) {
        throw std::runtime_error("bake outputs must be outside the source levels directory");
    }

    // Existing caches require an explicit request to update them.
    if (std::filesystem::exists(options.output) && !options.update) {
        throw std::runtime_error("choose a new output directory; previous bakes are preserved");
    }

    // Unbounded controller counts cannot reach admission or worker allocation.
    if (options.jobs > 256 || options.threads > 256 || options.bounces > 64) {
        throw std::runtime_error("pipeline CPU or transport limit exceeded");
    }

    background(options);
}

}  // namespace

Options arguments(std::span<const std::string> values) {
    Options options;

    // Flags and path arguments are resolved once before cache ownership is acquired.
    for (std::size_t index = 0; index < values.size(); ++index) {
        const auto& key = values[index];

        // The dispatch token has no option value.
        if (key == "--bake-all") {
            continue;
        }

        // Update mode permits verified replacement while preserving failed attempts.
        if (key == "--update") {
            options.update = true;
            continue;
        }

        // Dry-run must leave exports and caches untouched.
        if (key == "--dry-run") {
            options.dry_run = true;
            continue;
        }

        // An incomplete option cannot consume an unrelated default path.
        if (index + 1 == values.size()) {
            throw std::runtime_error("missing pipeline option value");
        }

        option(options, key, values[++index]);
    }

    resolve_paths(options);
    return options;
}

// ---- Export configuration ----

std::vector<Level> discover(const Options& options) {
    std::vector<Level> levels;
    const std::regex name("level_[0-9]+");

    // Validate every matching export before scheduling the first expensive scene load.
    for (const auto& entry : std::filesystem::directory_iterator(options.levels)) {
        // Only numbered exported level directories participate in automatic baking.
        if (!entry.is_directory() || !std::regex_match(entry.path().filename().string(), name)) {
            continue;
        }

        const auto path = std::filesystem::canonical(entry.path());

        // Each admitted level needs a local manifest inside the selected export root.
        if (path.parent_path() != options.levels
            || !std::filesystem::is_regular_file(path / "manifest.json")) {
            throw std::runtime_error("missing manifest or symlinked level outside export root");
        }

        Level level{path};

        // Export byte sizes approximate both scene footprint and level progress weight.
        for (const auto& file : std::filesystem::recursive_directory_iterator(path)) {
            // Only actual files contribute to scene memory reservations.
            if (file.is_regular_file()) {
                level.bytes += file.file_size();
            }
        }

        level.memory = std::max(std::uint64_t{1} << 30, level.bytes * 4);
        levels.push_back(level);
    }

    // Numeric names match the launcher's established level ordering, with stable spelling ties.
    std::sort(levels.begin(), levels.end(), [](const Level& a, const Level& b) {
        const auto first = a.path.filename().string();
        const auto second = b.path.filename().string();
        return std::pair{std::stoull(first.substr(6)), first}
               < std::pair{std::stoull(second.substr(6)), second};
    });

    // Discovery must produce a bounded nonempty admission schedule.
    if (levels.empty() || levels.size() > 256) {
        throw std::runtime_error("no exported levels or level count exceeds 256");
    }

    return levels;
}

json::Value region_settings(const std::filesystem::path& level) {
    const auto defaults = parsed(
        R"({"enabled":true,"interior_lighting":true,"coarse_bounds":"local_regions","coarse_padding":128,"padding":8,"vertical_padding":4,"max_probes":4096,"max_regions":1024,"no_geometry_coverage":false,"geometry_max_rise":32,"geometry_padding":3,"no_moby_seeds":false,"adaptive_density":false,"adaptive_relative_error":0.1,"adaptive_absolute_error":0.02})"
    );
    auto settings = defaults.members();
    const auto path = level / "probe-bake.json";

    // Missing per-level settings use the documented shared defaults.
    if (std::filesystem::exists(path)) {
        const auto profile = bake_json::document(path);

        // Malformed local controls cannot be interpreted as default coverage.
        if (profile["version"].number() != 1
            || (!profile["regions"].is_null() && !profile["regions"].is_object())) {
            throw std::runtime_error(
                "probe-bake.json requires version 1 and an object for regions"
            );
        }

        // Preserve each explicit local control; unknown keys are rejected below.
        for (const auto& [key, value] : profile["regions"].members()) {
            settings[key] = value;
        }
    }

    const auto result = bake_json::object_value(settings);
    validate_regions(result, defaults);

    // Scene placement uses complete geometry bounds without floor-seeded allocations or ceiling sources.
    if (load_level_bake_profile(level).strategy == BakeStrategy::Scene) {
        settings["enabled"] = parsed("false");
        settings["interior_lighting"] = parsed("false");
        settings["coarse_bounds"] = parsed("\"scene\"");
    }

    return bake_json::object_value(settings);
}

LevelBakeProfile profile(const std::filesystem::path& level, const Options& options) {
    auto result = load_level_bake_profile(level);

    // An explicit ray override must affect both transport and its semantic identity.
    if (options.rays) {
        result.activation.bake.rays_per_probe = options.rays;
    }

    // Direct-only transport uses zero bounces, so absence has a separate sentinel.
    if (options.bounces >= 0) {
        result.activation.bake.bounces = static_cast<unsigned>(options.bounces);
    }

    return result;
}
}  // namespace openrac::lighting::pipeline
