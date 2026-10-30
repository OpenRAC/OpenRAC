// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Native interior preparation: stage caching, bounded source plans and preservation of authored overrides.
#include "enhanced/tools/probes/interior_prepare.h"

#include <algorithm>
#include <format>
#include <map>
#include <sstream>
#include <stdexcept>

#include "assets/disc/checksum.h"
#include "common/json.h"
#include "common/profiler.h"
#include "enhanced/lighting/interior_lights.h"
#include "enhanced/tools/probes/interior_inputs.h"
#include "enhanced/tools/probes/interior_stage.h"
#include "enhanced/tools/probes/stage_file.h"

namespace openrac::lighting {
namespace {
// One authored endpoint mood, before the existing lighting module converts it to linear radiance.
struct Mood {
    int light = 0;                      // Authored light-set ID.
    std::array<unsigned, 3> ambient{};  // Encoded ambient RGB bytes.
    bool operator==(const Mood&) const = default;
};

// Describes the existing native default source controls, avoiding another copy of their constants.
std::string source_settings() {
    const InteriorLightSettings settings;
    return std::format(
        "{{\"emitter_boost\":{},\"emission_limit\":{},\"point_limit\":{},\"ceiling_limit\":{},"
        "\"ceiling_strength\":{},\"half_extent\":{},\"separation\":{},\"max_height\":{},"
        "\"max_lights\":{},\"max_per_region\":{}}}",
        settings.emitter_boost,
        settings.emission_limit,
        settings.point_limit,
        settings.ceiling_limit,
        settings.ceiling_strength,
        settings.half_extent,
        settings.separation,
        settings.max_height,
        settings.max_lights,
        settings.max_per_region
    );
}

// Reads required authored input; malformed explicit source files are never silently replaced.
json::Value read_json(const std::filesystem::path& path) {
    json::Value value;
    std::string error;

    // Missing optional files are represented by null, not by an authored override.
    if (!std::filesystem::exists(path)) {
        return value;
    }

    if (!json::parse_file(path.string(), value, error)) {
        throw std::runtime_error(error);
    }

    return value;
}

// Separates this stage's cache from transport rebuilds and captures default source changes.
std::string generation_identity(std::string_view source) {
    assets::disc::Sha256 hash;
    hash.update(source);
    hash.update(interior_stage_profile());
    hash.update(source_settings());
    // Bump the domain when native input decoding or source-plan selection changes.
    hash.update("native-interior-preparation-v1");
    return assets::disc::to_hex(hash.finish());
}

// Associates stable inference labels with their agreeing authored endpoint moods.
std::map<unsigned, Mood> label_moods(const InteriorInput& input, const interiors::Result& result) {
    std::map<unsigned, Mood> labels;

    for (const auto& seed : result.seeds) {
        const auto found = std::find_if(
            input.transitions.begin(), input.transitions.end(), [&](const auto& transition) {
                return transition.id == seed.transition;
            }
        );

        // Inference must retain valid references to the authored transition endpoints.
        if (found == input.transitions.end() || seed.side >= found->light.size()) {
            throw std::runtime_error("invalid inferred lighting endpoint");
        }

        const Mood mood{found->light[seed.side], found->ambient[seed.side]};
        const auto previous = labels.find(seed.label);

        // Conflicting labels cannot supply a reliable room mood.
        if (previous != labels.end() && previous->second != mood) {
            throw std::runtime_error("conflicting inference label");
        }

        labels[seed.label] = mood;
    }

    return labels;
}

// Keeps the established low-sky, unambiguous-component rule for generated interior sources.
std::string source_plan(const InteriorInput& input, const interiors::Result& result) {
    const auto labels = label_moods(input, result);
    std::ostringstream out;
    // Nine digits retain the native float positions without serialisation loss.
    out.precision(9);
    out << "{\"version\":1,\"level\":" << input.level << ",\"source_sha256\":\""
        << input.source_sha256
        << "\",\"generator\":\"openrac-interiors-v1\",\"generation_sha256\":\""
        << input.generation_sha256 << "\",\"settings\":" << source_settings() << ",\"samples\":[";
    bool first = true;

    for (const auto& cell : result.cells) {
        const auto& region = result.regions.at(cell.component);

        // Sky exposure above one fifth or conflicting labels do not establish an interior mood.
        if (region.labels.size() != 1 || cell.sky > .2f) {
            continue;
        }

        const auto& mood = labels.at(region.labels.front());

        // Separate records without a trailing comma.
        if (!first) {
            out << ',';
        }

        first = false;
        out << "{\"position\":[" << cell.position[0] << ',' << cell.position[1] << ','
            << cell.position[2] << "],\"region\":" << cell.component
            << ",\"light_set\":" << mood.light << ",\"ambient\":[" << mood.ambient[0] << ','
            << mood.ambient[1] << ',' << mood.ambient[2] << "]}";
    }

    out << "]}\n";
    return out.str();
}

// Existing reports are reusable only with matching source/algorithm identity and geometry diagnostics.
bool current_reports(const std::filesystem::path& output, std::string_view identity) {
    json::Value report;
    json::Value geometry;
    std::string error;
    return json::parse_file((output / "interior-inference.json").string(), report, error)
           && report["format"].number() == 1 && report["cells"].is_array()
           && report["generation_sha256"].string() == identity
           && json::parse_file((output / "interior-input.json").string(), geometry, error)
           && geometry["format"].number() == 1 && geometry["triangles"].is_array()
           && geometry["source_sha256"].string() == report["source_sha256"].string();
}

// Requires a successful native stage before its dependent output may be published.
void checked(Result<void> result) {
    // A failed write leaves the previous verified output intact.
    if (!result) {
        throw std::runtime_error(result.error().message);
    }
}
}  // namespace

Result<void> prepare_level_interiors(
    const std::filesystem::path& level,
    const std::filesystem::path& gameplay,
    const std::filesystem::path& output,
    bool generate_sources
) {
    try {
        profiling::Zone zone("Prepare level interiors");
        const auto source = interior_source_identity(level, gameplay);

        if (!source) {
            return unexpected(source.error());
        }

        const auto generation = generation_identity(*source);
        const auto sidecar = level / "probe-interiors.json";
        const auto previous = read_json(sidecar);
        const bool owned =
            generate_sources
            && (previous.is_null() || previous["generator"].string() == "openrac-interiors-v1");

        // Explicit overrides remain authoritative; report reuse still requires current geometry.
        if (current_reports(output, generation)
            && (!owned || previous["generation_sha256"].string() == generation)) {
            return {};
        }

        auto input = load_interior_input(level, gameplay);

        if (!input) {
            return unexpected(input.error());
        }

        input->generation_sha256 = generation;
        const interiors::Settings settings;
        const auto result = interiors::infer(input->triangles, input->transitions, settings);
        // Validate source selection before publishing any diagnostic or generated sidecar.
        const auto plan = owned ? source_plan(*input, result) : std::string{};
        checked(write_interior_input(output / "interior-input.json", *input));
        checked(
            write_interior_report(output / "interior-inference.json", *input, settings, result)
        );

        // Only this stage's owned generated file may be replaced.
        if (owned) {
            checked(write_stage_file(sidecar, plan));
        }

        return {};
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}
}  // namespace openrac::lighting
