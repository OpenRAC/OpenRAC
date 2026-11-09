// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Material property metadata: shared strict ranges and editor labels, without GPU dependencies.
#include <algorithm>
#include <cmath>
#include <limits>

#include "materials/catalog.h"
#include "materials/metadata_internal.h"

namespace openrac::materials {
namespace {
// Ranges mirror the existing material controls; emission radiance remains HDR.
constexpr Field kFields[] = {
    {"shading", "Shading", Kind::Text, 0, 0, false},
    {"surface.tint", "Diffuse tint", Kind::Colour, 0, 1, true},
    {"surface.diffuse_scale", "Diffuse brightness", Kind::Number, 0, 1, true},
    {"surface.roughness", "Roughness", Kind::Number, 0.04, 1, false},
    {"surface.reflectivity", "Reflectivity (F0)", Kind::Number, 0, 1, false},
    {"reflection.strength", "Reflection strength", Kind::Number, 0, 4, false},
    {"reflection.tint", "Reflection tint", Kind::Colour, 0, 1, false},
    {"reflection.opacity", "Reflection opacity", Kind::Number, 0, 1, false},
    {"emission.runtime_intensity",
     "Emission intensity",
     Kind::Number,
     0,
     std::numeric_limits<float>::max(),
     true},
    {"emission.bake_intensity",
     "Baked emission intensity",
     Kind::Number,
     0,
     std::numeric_limits<float>::max(),
     true},
    {"emission.contrast", "Emission contrast", Kind::Number, 1, 4, true},
    {"emission.vertex_colour", "Vertex colour emission", Kind::Boolean, 0, 1, true},
    {"emission.average_for_bake", "Average baked emission per triangle", Kind::Boolean, 0, 1, true},
    {"emission.bake_mode", "Baked emission mode", Kind::Text, 0, 0, true},
    {"emission.bake_mesh_rgb",
     "Mesh emission (linear RGB)",
     Kind::Colour,
     0,
     std::numeric_limits<float>::max(),
     true},
    {"emission.bake_use_cutout", "Mask baked emission with cutouts", Kind::Boolean, 0, 1, true},
    {"emission.use_base_texture", "Use colour texture", Kind::Boolean, 0, 1, true},
    {"emission.texture", "Emission texture", Kind::Text, 0, 0, true},
    {"foliage.transmission", "Transmission", Kind::Number, 0, 1, false},
    {"foliage.thickness", "Optical thickness", Kind::Number, 0, 4, false},
    {"foliage.normal_bend", "Normal bend", Kind::Number, 0, 1, false},
    {"fur.enabled", "Fur detail", Kind::Boolean, 0, 1, false},
    {"fur.strength", "Detail strength", Kind::Number, 0, 1, false},
    {"fur.contrast", "Strand contrast", Kind::Number, 0, 0.15, false},
    {"fur.fade_distance", "Detail fade distance", Kind::Number, 8, 160, false},
    {"fur.lighting_enabled", "Highlights and scattering", Kind::Boolean, 0, 1, false},
    {"fur.highlight", "Highlight strength", Kind::Number, 0, 2, false},
    {"fur.softness", "Highlight softness", Kind::Number, 0.35, 1, false},
    {"fur.directionality", "Highlight directionality", Kind::Number, 0, 0.9, false},
    {"fur.scattering", "Light scattering", Kind::Number, 0, 0.5, false},
    {"fur.environment_strength", "Environment response", Kind::Number, 0, 4, false},
    {"fur.profile", "Coverage profile", Kind::Text, 0, 0, false},
    {"wetness.diffuse_scale", "Wet diffuse brightness", Kind::Number, 0, 1, false},
    {"wetness.specular_strength", "Wet specular strength", Kind::Number, 0, 4, false},
    {"wetness.dry_roughness", "Dry roughness", Kind::Number, 0.08, 1, false},
    {"wetness.wet_roughness", "Wet roughness", Kind::Number, 0.08, 1, false},
    {"wetness.reflectivity", "Wet reflectivity (F0)", Kind::Number, 0, 0.16, false},
};

// Relative asset references cannot escape the exported level or select an absolute host path.
bool valid_reference(std::string_view text) {
    return !text.empty() && text.front() != '/' && text.front() != '\\'
           && text.find(':') == text.npos && text.find("..") == text.npos;
}

// Text properties are deliberately bounded to supported shader families and local asset references.
bool valid_text(std::string_view key, const std::string& value) {
    if (key == "emission.bake_mode") {
        return value == "texture" || value == "mesh";
    }
    // Shader selection cannot enable arbitrary programs or alter original coverage flags.
    if (key == "shading") {
        constexpr std::string_view kNames[] =
            {"standard", "foliage", "water", "chrome", "glass", "stylised"};
        return std::find(std::begin(kNames), std::end(kNames), value) != std::end(kNames);
    }
    return valid_reference(value);
}

// Checks one representation before the resolver or an editor can publish it.
bool valid_value(const Field& spec, const Value& value) {
    switch (spec.kind) {
        // Boolean controls accept no numeric substitutions.
        case Kind::Boolean:
            return std::holds_alternative<bool>(value);
        // Numeric bounds are checked before narrowing to shader floats.
        case Kind::Number: {
            const auto* number = std::get_if<double>(&value);
            return number && std::isfinite(*number) && *number >= spec.minimum
                   && *number <= spec.maximum;
        }
        // Every colour channel uses the same finite range.
        case Kind::Colour: {
            const auto* colour = std::get_if<std::array<double, 3>>(&value);
            return colour && std::all_of(colour->begin(), colour->end(), [&](double channel) {
                       return std::isfinite(channel) && channel >= spec.minimum
                              && channel <= spec.maximum;
                   });
        }
        // Text references have a small fixed vocabulary or remain level-relative.
        case Kind::Text: {
            const auto* text = std::get_if<std::string>(&value);
            return text && valid_text(spec.path, *text);
        }
    }
    return false;
}

// Volatile conversion uses the selected hardware rounding mode on every supported compiler without locale-dependent parsing.
float narrow(double value, float fallback) {
    if (!std::isfinite(value) || std::abs(value) > std::numeric_limits<float>::max()) {
        return fallback;
    }
    const MetadataRounding rounding;
    const volatile double input = value;
    return static_cast<float>(input);
}

}  // namespace

float number_value(const Properties& properties, std::string_view key, float fallback) {
    const auto found = properties.find(key);
    return found == properties.end() ? fallback : narrow(std::get<double>(found->second), fallback);
}

bool bool_value(const Properties& properties, std::string_view key, bool fallback) {
    const auto found = properties.find(key);
    return found == properties.end() ? fallback : std::get<bool>(found->second);
}

std::string_view text_value(
    const Properties& properties, std::string_view key, std::string_view fallback
) {
    const auto found = properties.find(key);
    return found == properties.end() ? fallback
                                     : std::string_view(std::get<std::string>(found->second));
}

std::array<float, 3> colour_value(
    const Properties& properties, std::string_view key, std::array<float, 3> fallback
) {
    const auto found = properties.find(key);
    if (found == properties.end()) {
        return fallback;
    }
    const auto& value = std::get<std::array<double, 3>>(found->second);
    for (std::size_t channel = 0; channel < fallback.size(); ++channel) {
        fallback[channel] = narrow(value[channel], fallback[channel]);
    }
    return fallback;
}

std::span<const Field> fields() {
    return kFields;
}

const Field* field(std::string_view path) {
    const auto found =
        std::find_if(std::begin(kFields), std::end(kFields), [&](const Field& candidate) {
            return candidate.path == path;
        });
    return found == std::end(kFields) ? nullptr : found;
}

Result<void> validate(const Properties& properties) {
    // One invalid component rejects the complete edit, retaining the previous binding.
    for (const auto& [key, value] : properties) {
        const auto* spec = field(key);
        if (!spec || !valid_value(*spec, value)) {
            return unexpected(Error{"invalid material property: " + key});
        }
    }
    const auto shading = properties.find("shading");
    const auto roughness = properties.find("surface.roughness");
    // Thin-leaf curvature was reviewed for broad GGX lobes; preserve its existing roughness floor.
    if (shading != properties.end() && shading->second == Value{std::string("foliage")}
        && roughness != properties.end() && std::get<double>(roughness->second) < .35) {
        return unexpected(Error{"foliage roughness must be at least 0.35"});
    }
    return {};
}

Properties transport_properties(const Properties& properties) {
    Properties result;
    // Only effective transport controls contribute, never debug, fur, wetness or reflection tuning.
    for (const auto& [key, value] : properties) {
        const auto* spec = field(key);
        if (spec && spec->transport) {
            result.emplace(key, value);
        }
    }
    // An explicit bake intensity decouples visible emission intensity from static transport.
    if (result.contains("emission.bake_intensity")) {
        result.erase("emission.runtime_intensity");
    }
    return result;
}
}  // namespace openrac::materials
