// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Native interior preparation: stage caching, bounded source plans and preservation of authored overrides.
#include "enhanced/tools/baker/sources/interior_prepare.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>

#include "assets/disc/checksum.h"
#include "common/json.h"
#include "common/log.h"
#include "common/profiler.h"
#include "enhanced/lighting/interior_lights.h"
#include "enhanced/tools/baker/pipeline/bake_json.h"
#include "enhanced/tools/baker/pipeline/stage_file.h"
#include "enhanced/tools/baker/sources/interior_inputs.h"
#include "enhanced/tools/baker/sources/interior_sources.h"
#include "enhanced/tools/baker/sources/interior_stage.h"

namespace openrac::lighting {
namespace {
// One authored endpoint mood, before the existing lighting module converts it to linear radiance.
struct Mood {
    int light = 0;                      // Authored light-set ID.
    std::array<unsigned, 3> ambient{};  // Encoded ambient RGB bytes.
    interiors::Point up{0, 0, 1};       // Ceiling search axis.
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

// Reads a finite three-component authoring coordinate.
interiors::Point read_point(const json::Value& value) {
    if (value.items().size() != 3) {
        throw std::runtime_error("lighting-environments.json requires three coordinates per point");
    }
    interiors::Point point{};
    for (unsigned axis = 0; axis < 3; ++axis) {
        const double component = value[axis].number(std::numeric_limits<double>::quiet_NaN());
        if (!std::isfinite(component) || std::abs(component) > std::numeric_limits<float>::max()) {
            throw std::runtime_error("lighting-environments.json contains an invalid coordinate");
        }
        point[axis] = static_cast<float>(component);
    }
    return point;
}

// Reads a bounded integer without silently truncating an authored value.
int read_integer(const json::Value& value, int maximum) {
    const double number = value.number(-1);
    if (!(number >= 0 && number <= maximum && std::floor(number) == number)) {
        throw std::runtime_error("lighting-environments.json contains an invalid light or colour");
    }
    return static_cast<int>(number);
}

// A named frame owns one normalized deck normal shared by all its sources.
struct AuthoredFrame {
    unsigned id;          // Zero is reserved for the implicit world frame.
    interiors::Point up;  // Unit deck normal.
};

// Reads triangulated lighting-only polygons and enclosed-air mood sources.
interiors::Settings read_authoring(const std::filesystem::path& path) {
    interiors::Settings settings;
    if (!std::filesystem::exists(path)) {
        return settings;
    }
    const auto data = read_json(path);
    if (data["version"].number() != 1 || !data["barriers"].is_array()
        || !data["sources"].is_array()) {
        throw std::runtime_error(
            "lighting-environments.json requires version 1, barriers and sources"
        );
    }
    const auto& definitions = data["frames"];
    if (!definitions.is_null() && !definitions.is_object()) {
        throw std::runtime_error("lighting-environments.json frames must be an object");
    }
    std::map<std::string, AuthoredFrame> frames;
    for (const auto& [name, definition] : definitions.members()) {
        const auto up = read_point(definition["up"]);
        const auto length = std::hypot(up[0], up[1], up[2]);
        if (name.empty() || !std::isfinite(length) || up[2] <= 0 || length < 1e-6f) {
            throw std::runtime_error(
                "lighting-environments.json frame requires a name and an upward vector"
            );
        }
        const auto id = static_cast<unsigned>(frames.size()) + 1;
        frames.emplace(name, AuthoredFrame{id, {up[0] / length, up[1] / length, up[2] / length}});
    }
    unsigned barrier_index = 0;
    for (const auto& triangle : data["barriers"].items()) {
        if (triangle.items().size() != 3) {
            throw std::runtime_error("lighting-environments.json barriers require three vertices");
        }
        const auto a = read_point(triangle[0]);
        const auto b = read_point(triangle[1]);
        const auto c = read_point(triangle[2]);
        const std::array<double, 3> u{
            double(b[0]) - a[0], double(b[1]) - a[1], double(b[2]) - a[2]
        };
        const std::array<double, 3> v{
            double(c[0]) - a[0], double(c[1]) - a[1], double(c[2]) - a[2]
        };
        if (u[1] * v[2] == u[2] * v[1] && u[2] * v[0] == u[0] * v[2]
            && u[0] * v[1] == u[1] * v[0]) {
            throw std::runtime_error(
                std::format("lighting-environments.json barrier {} has zero area", barrier_index)
            );
        }
        settings.lighting_barriers.push_back({{a, b, c}, false});
        ++barrier_index;
    }
    for (const auto& source : data["sources"].items()) {
        const auto& ambient = source["ambient"];
        if (ambient.items().size() != 3) {
            throw std::runtime_error("lighting-environments.json ambient requires three RGB bytes");
        }
        AuthoredFrame frame{0, {0, 0, 1}};
        if (!source["up"].is_null()) {
            throw std::runtime_error(
                "lighting-environments.json source up belongs in a named frame"
            );
        }
        if (!source["frame"].is_null()) {
            const auto found = frames.find(source["frame"].string());
            if (found == frames.end()) {
                throw std::runtime_error(
                    "lighting-environments.json source refers to an unknown frame"
                );
            }
            frame = found->second;
        }
        settings.lighting_sources.push_back(
            {read_point(source["position"]),
             read_integer(source["light"], kInteriorLightSetCount - 1),
             {static_cast<unsigned>(read_integer(ambient[0], 255)),
              static_cast<unsigned>(read_integer(ambient[1], 255)),
              static_cast<unsigned>(read_integer(ambient[2], 255))},
             frame.up,
             frame.id}
        );
    }
    return settings;
}

// Separates this stage's cache from transport rebuilds and captures default source changes.
std::string generation_identity(std::string_view source, const std::filesystem::path& authoring) {
    assets::disc::Sha256 hash;
    hash.update(source);
    // Keep the established cache identity for levels without authoring.
    if (std::filesystem::exists(authoring)) {
        hash.update("lighting-environments-v4");
        bake_json::hash_file(hash, authoring);
    }
    hash.update(interior_stage_profile());
    hash.update(source_settings());
    // Bump the domain when native input decoding or source-plan selection changes.
    hash.update("native-interior-preparation-v1");
    return assets::disc::to_hex(hash.finish());
}

// Associates stable inference labels with their agreeing authored endpoint moods.
std::map<unsigned, Mood> label_moods(
    const InteriorInput& input, const interiors::Settings& settings, const interiors::Result& result
) {
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

    for (std::size_t index = 0; index < result.source_labels.size(); ++index) {
        const unsigned label = result.source_labels[index];
        if (label != ~0u) {
            const auto& source = settings.lighting_sources[index];
            labels.emplace(label, Mood{source.light, source.ambient, source.up});
        }
    }
    return labels;
}

// Keeps the established low-sky, unambiguous-component rule for generated interior sources.
std::string source_plan(
    const InteriorInput& input, const interiors::Settings& settings, const interiors::Result& result
) {
    const auto labels = label_moods(input, settings, result);
    std::ostringstream out;
    // Nine digits retain the native float positions without serialisation loss.
    out.precision(9);
    out << "{\"version\":1,\"level\":" << input.level << ",\"source_sha256\":\""
        << input.source_sha256
        << "\",\"generator\":\"openrac-interiors-v1\",\"generation_sha256\":\""
        << input.generation_sha256 << "\",\"settings\":" << source_settings() << ",\"samples\":[";
    bool first = true;

    for (std::size_t i = 0; i < result.cells.size(); ++i) {
        const auto& cell = result.cells[i];
        const auto& region = result.regions.at(cell.component);
        const unsigned authored = result.source_cell_labels.empty()
                                      ? interiors::kUnreachedSourceCell
                                      : result.source_cell_labels[i];
        const unsigned label =
            authored == interiors::kUnreachedSourceCell && region.labels.size() == 1
                ? region.labels.front()
                : authored;

        // Sky exposure above one fifth or conflicting labels do not establish an interior mood.
        if (cell.sky > .2f || label == interiors::kUnreachedSourceCell
            || label == interiors::kConflictingSourceCell) {
            continue;
        }

        const auto& mood = labels.at(label);

        // Separate records without a trailing comma.
        if (!first) {
            out << ',';
        }

        first = false;
        out << "{\"position\":[" << cell.position[0] << ',' << cell.position[1] << ','
            << cell.position[2] << "],\"region\":" << cell.component
            << ",\"light_set\":" << mood.light << ",\"ambient\":[" << mood.ambient[0] << ','
            << mood.ambient[1] << ',' << mood.ambient[2] << ']';
        if (mood.up != interiors::Point{0, 0, 1}) {
            out << ",\"up\":[" << mood.up[0] << ',' << mood.up[1] << ',' << mood.up[2] << ']';
        }
        out << '}';
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

Result<bool> prepare_level_interiors(
    const std::filesystem::path& level,
    const std::filesystem::path& gameplay,
    const std::filesystem::path& output,
    bool generate_sources,
    const std::filesystem::path& overrides
) {
    try {
        profiling::Zone zone("Prepare level interiors");
        const auto source = interior_source_identity(level, gameplay, overrides);

        if (!source) {
            return unexpected(source.error());
        }

        const auto authoring = level / "lighting-environments.json";
        const auto generation = generation_identity(*source, authoring);
        const auto original = level / "probe-interiors.json";
        const auto sidecar = (overrides.empty() ? level : output) / "probe-interiors.json";
        const auto previous = read_json(sidecar);
        const auto authored = read_json(original);
        const bool explicit_sources =
            !authored.is_null() && authored["generator"].string() != "openrac-interiors-v1";
        const bool owned =
            generate_sources && !explicit_sources
            && (previous.is_null() || previous["generator"].string() == "openrac-interiors-v1");

        // Explicit overrides remain authoritative; report reuse still requires current geometry.
        if (current_reports(output, generation)
            && (!owned || previous["generation_sha256"].string() == generation)) {
            return false;
        }

        auto input = load_interior_input(level, gameplay, overrides);

        if (!input) {
            return unexpected(input.error());
        }

        input->generation_sha256 = generation;
        const auto settings = read_authoring(authoring);
        const auto result = interiors::infer(input->triangles, input->transitions, settings);
        // Validate source selection before publishing any diagnostic or generated sidecar.
        for (std::size_t index = 0; index < result.source_labels.size(); ++index) {
            if (result.source_labels[index] == ~0u) {
                log::warn("lighting-environments.json source {} did not reach enclosed air", index);
            }
        }
        const auto plan = owned ? source_plan(*input, settings, result) : std::string{};
        checked(write_interior_input(output / "interior-input.json", *input));
        checked(
            write_interior_report(output / "interior-inference.json", *input, settings, result)
        );

        // Compare bytes, not timestamps: unchanged generated text can retain the earlier snapshot.
        bool changed = false;
        if (owned) {
            assets::disc::Sha256 digest;
            digest.update(plan);
            changed = !std::filesystem::is_regular_file(sidecar)
                      || bake_json::file_hash(sidecar) != assets::disc::to_hex(digest.finish());
            checked(write_stage_file(sidecar, plan));
        }

        return changed;
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}
}  // namespace openrac::lighting
