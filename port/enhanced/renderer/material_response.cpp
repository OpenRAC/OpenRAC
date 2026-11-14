// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// material_runtime.h's typed conversion, shared by runtime and material fixtures.
#include "enhanced/renderer/material_runtime.h"

namespace openrac::enhanced {
namespace {
// Uses the schema's canonical FP32 conversion rather than inheriting game rounding modes.
float number(const materials::Properties& properties, std::string_view key, float fallback) {
    return materials::number_value(properties, key, fallback);
}

// Compiles verified fur appearance; the coverage asset and original alpha remain separate.
FurMaterialSettings fur_response(const materials::Properties& properties) {
    FurMaterialSettings result;
    result.enabled = materials::bool_value(properties, "fur.enabled", result.enabled);
    result.lighting_enabled =
        materials::bool_value(properties, "fur.lighting_enabled", result.lighting_enabled);
    result.strength = number(properties, "fur.strength", result.strength);
    result.contrast = number(properties, "fur.contrast", result.contrast);
    result.fade_distance = number(properties, "fur.fade_distance", result.fade_distance);
    result.highlight = number(properties, "fur.highlight", result.highlight);
    result.softness = number(properties, "fur.softness", result.softness);
    result.directionality = number(properties, "fur.directionality", result.directionality);
    result.scattering = number(properties, "fur.scattering", result.scattering);
    result.environment_strength =
        number(properties, "fur.environment_strength", result.environment_strength);
    return result;
}

// Selects the established family defaults before applying sparse optical properties.
ReflectionMaterialSettings reflection_response(
    const scene::MaterialSource& source, const materials::Properties& properties
) {
    ReflectionMaterialSettings result;
    // Legacy assets retain their established family until their authored catalog is exported.
    result = reflection_material_for(source.identity.material_name, source.identity.class_id);
    const auto family = materials::text_value(properties, "shading");
    if (family == "water" || source.identity.material_name == "@water") {
        result = default_reflection_material(ReflectionSurface::Water);
    } else if (family == "glass") {
        result = default_reflection_material(ReflectionSurface::Glass);
    } else if (family == "stylised") {
        result = default_reflection_material(ReflectionSurface::Stylised);
    } else if (family == "chrome") {
        result = default_reflection_material(ReflectionSurface::Chrome);
    } else if (family == "foliage") {
        result.roughness = .65f;
        result.reflectivity = .04f;
    } else if (!source.identity.metal) {
        result.roughness = .5f;
        result.reflectivity = .04f;
    }
    auto& reflection = result;
    reflection.roughness = number(properties, "surface.roughness", reflection.roughness);
    reflection.reflectivity = number(properties, "surface.reflectivity", reflection.reflectivity);
    reflection.strength = number(properties, "reflection.strength", reflection.strength);
    reflection.opacity = number(properties, "reflection.opacity", reflection.opacity);
    reflection.tint = materials::colour_value(properties, "reflection.tint", reflection.tint);
    return result;
}
}  // namespace

MaterialResponse material_response(
    const scene::MaterialSource& source, const materials::Properties& properties
) {
    MaterialResponse result;
    const auto type = materials::text_value(properties, "type");
    result.original = type == "original";
    result.glass_surface = type == "glass-lit";
    result.wet_fur = type == "lit-wet-fur";
    if (result.original) {
        result.fallback_channels = 4;
        result.family_override = true;
        result.fur_override = true;
        result.fur.enabled = false;
        result.fur.lighting_enabled = false;
        return result;
    }
    result.family_override = properties.contains("shading");
    constexpr int kDiffuse = 1;
    constexpr int kEmission = 2;
    if (properties.contains("surface.diffuse_scale")) {
        result.fallback_channels |= kDiffuse;
    }
    if (properties.contains("emission.runtime_intensity") || properties.contains("emission.texture")
        || properties.contains("emission.contrast") || properties.contains("emission.vertex_colour")
        || properties.contains("emission.use_base_texture")) {
        result.fallback_channels |= kEmission;
    }
    result.sky_tint = materials::colour_value(properties, "surface.tint", result.sky_tint);
    result.reflection = reflection_response(source, properties);
    result.glass.transmission = number(properties, "glass.transmission", result.glass.transmission);
    result.glass.tint = materials::colour_value(properties, "glass.tint", result.glass.tint);
    const auto reflection = properties.lower_bound("reflection.");
    result.metallic = number(properties, "surface.metallic", -1.f);
    const bool optical_override =
        result.metallic >= 0.f || properties.contains("surface.roughness")
        || properties.contains("surface.reflectivity")
        || (reflection != properties.end() && reflection->first.starts_with("reflection."));
    result.reflection_override = result.family_override || optical_override;
    const auto family = materials::text_value(properties, "shading");
    result.dielectric_specular = !source.identity.metal && optical_override
                                 && (family.empty() || family == "standard" || family == "foliage");
    result.fur = fur_response(properties);
    const auto fur = properties.lower_bound("fur.");
    result.fur_override = fur != properties.end() && fur->first.starts_with("fur.");
    if (!type.empty()) {
        result.fur_override = true;
        result.fur.enabled &= type == "lit-fur" || type == "lit-wet-fur";
        result.fur.lighting_enabled &= result.fur.enabled;
    }
    const auto wet = properties.lower_bound("wetness.");
    result.wetness_override = wet != properties.end() && wet->first.starts_with("wetness.");
    auto& wetness = result.wetness;
    wetness.diffuse_scale = number(properties, "wetness.diffuse_scale", wetness.diffuse_scale);
    wetness.specular_strength =
        number(properties, "wetness.specular_strength", wetness.specular_strength);
    wetness.dry_roughness = number(properties, "wetness.dry_roughness", wetness.dry_roughness);
    wetness.wet_roughness = number(properties, "wetness.wet_roughness", wetness.wet_roughness);
    wetness.reflectivity = number(properties, "wetness.reflectivity", wetness.reflectivity);
    // Explicit family selection never changes original coverage or reflective packet eligibility.
    if (materials::text_value(properties, "shading") == "foliage" && !source.identity.metal) {
        result.reflection.reflectivity = number(properties, "surface.reflectivity", .04f);
        FoliageMaterial foliage;
        foliage.transmission = number(properties, "foliage.transmission", foliage.transmission);
        foliage.thickness = number(properties, "foliage.thickness", foliage.thickness);
        foliage.normal_bend = number(properties, "foliage.normal_bend", foliage.normal_bend);
        foliage.roughness = number(properties, "surface.roughness", foliage.roughness);
        result.foliage = pack_foliage_material(foliage);
    }
    return result;
}
}  // namespace openrac::enhanced
