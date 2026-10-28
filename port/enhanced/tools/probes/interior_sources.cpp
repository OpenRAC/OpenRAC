// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Interior source sidecar validation, authored mood references and diagnostic serialization.

#include "enhanced/tools/probes/interior_sources.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>

#include "common/json.h"
#include "enhanced/lighting/interior_lights.h"
#include "scene/level.h"

namespace openrac::lighting {
namespace {
// RGB dimensions and light-bank layout follow export_probe_scene (seen in game code).
constexpr unsigned kChannels = 3;
constexpr unsigned kLightSetFloats = 16;
constexpr unsigned kLightCount = 2;
constexpr unsigned kLightStride = 8;
constexpr float kAmbientScale = 128;
constexpr float kDirectionalWeight = .5f;

// Reads an explicitly typed finite scalar rather than accepting JSON's wrong-type fallback.
float scalar(const json::Value& value) {
    if (!value.is_number() || !std::isfinite(value.number())) {
        throw std::runtime_error("interior source requires finite numeric values");
    }

    return static_cast<float>(value.number());
}

// Reads bounded integer indices before narrowing an untrusted JSON number.
unsigned index(const json::Value& value, unsigned limit) {
    const float number = scalar(value);

    if (number < 0 || number > static_cast<float>(limit) || std::floor(number) != number) {
        throw std::runtime_error("invalid interior source index");
    }

    return static_cast<unsigned>(number);
}

// Reads a three-component world position or encoded ambient colour.
Float3 vector(const json::Value& value) {
    if (!value.is_array() || value.items().size() != kChannels) {
        throw std::runtime_error("interior source requires three components");
    }

    return {scalar(value[0]), scalar(value[1]), scalar(value[2])};
}

// Converts an encoded artistic lighting reference to linear RGB; this is not a pixel calibration.
float linear_reference(float encoded) {
    constexpr float kLinearThreshold = .04045f;
    constexpr float kLinearDivisor = 12.92f;
    constexpr float kOffset = .055f;
    constexpr float kScale = 1.055f;
    constexpr float kExponent = 2.4f;
    encoded = std::clamp(encoded, 0.f, 1.f);
    return encoded <= kLinearThreshold ? encoded / kLinearDivisor
                                       : std::pow((encoded + kOffset) / kScale, kExponent);
}

// Combines the authored ambient and directional colours into a loose mood reference.
InteriorLightSeed read_seed(const json::Value& row, const scene::LevelData& level) {
    InteriorLightSeed seed;
    seed.position = vector(row["position"]);
    seed.region = index(row["region"], 100'000);
    const unsigned light_set = index(row["light_set"], kLightSetFloats - 1);
    const auto ambient = vector(row["ambient"]);
    const unsigned bank = light_set * kLightSetFloats;

    // Each reference retains RGB rather than reducing the original lighting to luminance.
    for (unsigned channel = 0; channel < kChannels; ++channel) {
        if (ambient[channel] < 0 || ambient[channel] > 255) {
            throw std::runtime_error("invalid interior ambient reference");
        }

        float encoded = ambient[channel] / kAmbientScale;

        for (unsigned light = 0; light < kLightCount; ++light) {
            encoded +=
                kDirectionalWeight * level.light_sets.at(bank + light * kLightStride + channel);
        }

        seed.colour[channel] = linear_reference(encoded);
    }

    return seed;
}

// Resolves only named artistic controls; source limits are validated by the lighting module.
InteriorLightSettings read_settings(const json::Value& values) {
    if (!values.is_object()) {
        throw std::runtime_error("interior source settings must be an object");
    }

    InteriorLightSettings settings;

    // An explicit table keeps misspelled controls from silently retaining their defaults.
    for (const auto& [name, value] : values.members()) {
        if (name == "emitter_boost") {
            settings.emitter_boost = scalar(value);
        } else if (name == "emission_limit") {
            settings.emission_limit = scalar(value);
        } else if (name == "point_limit") {
            settings.point_limit = scalar(value);
        } else if (name == "ceiling_limit") {
            settings.ceiling_limit = scalar(value);
        } else if (name == "ceiling_strength") {
            settings.ceiling_strength = scalar(value);
        } else if (name == "half_extent") {
            settings.half_extent = scalar(value);
        } else if (name == "separation") {
            settings.separation = scalar(value);
        } else if (name == "max_height") {
            settings.max_height = scalar(value);
        } else if (name == "max_lights") {
            settings.max_lights = index(value, kMaximumInteriorLightsPerLevel);
        } else if (name == "max_per_region") {
            settings.max_per_region = index(value, kMaximumInteriorLightsPerRegion);
        } else {
            throw std::runtime_error("unknown interior source setting: " + name);
        }
    }

    return settings;
}

// Serializes generated source positions and colours only into the caller's ignored bake report.
std::string describe(const InteriorLightReport& report) {
    std::ostringstream output;
    output.precision(9);
    output << "{\"version\":1,\"boosted_surfaces\":" << report.boosted_surfaces
           << ",\"boosted_points\":" << report.boosted_points << ",\"capped\":" << report.capped
           << ",\"rejected\":" << report.rejected << ",\"budget_skipped\":" << report.budget_skipped
           << ",\"lights\":[";

    // Stable order permits comparing two experiment reports without matching geometry by proximity.
    for (std::size_t n = 0; n < report.lights.size(); ++n) {
        if (n != 0) {
            output << ',';
        }

        const auto& light = report.lights[n];
        const auto& receiver = report.receivers[n];
        const auto& target = report.targets[n];
        const auto& existing = report.existing[n];
        output << "{\"position\":[" << light.position[0] << ',' << light.position[1] << ','
               << light.position[2] << "],\"radiance\":[" << light.radiance[0] << ','
               << light.radiance[1] << ',' << light.radiance[2] << "],\"axis_u\":["
               << light.axis_u[0] << ',' << light.axis_u[1] << ',' << light.axis_u[2]
               << "],\"axis_v\":[" << light.axis_v[0] << ',' << light.axis_v[1] << ','
               << light.axis_v[2] << "],\"receiver\":[" << receiver[0] << ',' << receiver[1] << ','
               << receiver[2] << "],\"target\":[" << target[0] << ',' << target[1] << ','
               << target[2] << "],\"existing\":[" << existing[0] << ',' << existing[1] << ','
               << existing[2] << "]}";
    }

    output << "]}\n";
    return output.str();
}
}  // namespace

Result<std::string> apply_interior_sources(const scene::LevelData& level, BakeScene& scene) {
    // Optional experiment inputs leave all ordinary levels on the existing bake path.
    try {
        const auto path = level.dir / "probe-interiors.json";

        if (!std::filesystem::exists(path)) {
            return std::string{};
        }

        json::Value data;
        std::string error;

        if (!json::parse_file(path.string(), data, error)) {
            return unexpected(Error{error});
        }

        if (data["version"].number() != 1 || data["level"].number(-1) != level.level
            || !data["samples"].is_array() || data["samples"].items().size() > 100'000
            || level.light_sets.size() != std::size_t{kLightSetFloats} * kLightSetFloats) {
            return unexpected(Error{"invalid interior source sidecar"});
        }

        std::vector<InteriorLightSeed> seeds;

        for (const auto& sample : data["samples"].items()) {
            seeds.push_back(read_seed(sample, level));
        }

        auto report = prepare_interior_lighting(scene, seeds, read_settings(data["settings"]));

        if (!report) {
            return unexpected(report.error());
        }

        return describe(*report);
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}
}  // namespace openrac::lighting
