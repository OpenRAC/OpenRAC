// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Compiles neutral material overrides for scene/bake bindings, preserving original coverage and packet flags.
#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "scene/material_overrides.h"

namespace openrac::scene {
namespace {
// Applies only supplied scalars; absent fields retain the original exported value.
void scalar(const materials::Properties& properties, std::string_view key, float& target) {
    const auto found = properties.find(key);
    if (found != properties.end()) {
        target = materials::number_value(properties, key, target);
    }
}

// Applies an optional checkbox without replacing unrelated original surface state.
void boolean(const materials::Properties& properties, std::string_view key, bool& target) {
    const auto found = properties.find(key);
    if (found != properties.end()) {
        target = std::get<bool>(found->second);
    }
}

// Copies visible/base properties while keeping alpha, image identity, wrapping and render passes intact.
Result<Material> surface(
    const Material& original, const materials::Properties& properties, const MaterialAssets& assets
) {
    Material result = original;
    const auto tint = properties.find("surface.tint");
    if (tint != properties.end()) {
        const auto value = materials::colour_value(properties, "surface.tint", {1, 1, 1});
        for (std::size_t channel = 0; channel < value.size(); ++channel) {
            result.colour[channel] *= value[channel];
        }
    }
    scalar(properties, "surface.diffuse_scale", result.diffuse_scale);
    const auto runtime = properties.find("emission.runtime_intensity");
    if (runtime != properties.end()) {
        result.emission.fill(materials::number_value(properties, "emission.runtime_intensity", 0));
        if (result.emission_image < 0) {
            result.emission_image = original.image;
        }
    }
    const auto bake = properties.find("emission.bake_intensity");
    if (bake != properties.end()) {
        result.bake_intensity = materials::number_value(properties, "emission.bake_intensity", 0);
        if (result.emission_image < 0) {
            result.emission_image = original.image;
        }
    }
    boolean(properties, "emission.vertex_colour", result.emission_vertex_colour);
    boolean(properties, "emission.average_for_bake", result.average_emission_for_bake);
    const auto bake_mode = materials::text_value(properties, "emission.bake_mode");
    if (!bake_mode.empty()) {
        result.average_emission_for_bake = false;
        result.bake_use_cutout = bake_mode == "texture";
        result.bake_mesh_rgb.reset();
        if (bake_mode == "mesh") {
            if (!properties.contains("emission.bake_mesh_rgb")) {
                return unexpected(Error{"mesh emission requires emission.bake_mesh_rgb"});
            }
            result.bake_mesh_rgb =
                materials::colour_value(properties, "emission.bake_mesh_rgb", {});
        }
    }
    boolean(properties, "emission.bake_use_cutout", result.bake_use_cutout);
    scalar(properties, "emission.contrast", result.emission_contrast);
    const auto base = properties.find("emission.use_base_texture");
    if (base != properties.end()) {
        result.emission_image = std::get<bool>(base->second) ? result.image : -1;
    }
    const auto texture = properties.find("emission.texture");
    if (texture != properties.end()) {
        const auto found = assets.images.find(std::get<std::string>(texture->second));
        if (found == assets.images.end()) {
            return unexpected(
                Error{
                    "material emission texture is not loaded: "
                    + std::get<std::string>(texture->second)
                }
            );
        }
        result.emission_image = found->second;
    }
    return result;
}

// Compares effective bake energy rather than visible emission when a fixed bake intensity is present.
std::array<float, 3> baked_emission(const Material& material) {
    if (material.bake_mesh_rgb) {
        return *material.bake_mesh_rgb;
    }
    auto value = material.emission;
    if (material.bake_intensity) {
        value.fill(*material.bake_intensity);
    }
    return value;
}

// Runtime-only properties and visible emission with a fixed bake intensity cannot invalidate static GI.
bool transport_changed(const Material& original, const Material& current) {
    for (std::size_t channel = 0; channel < 3; ++channel) {
        if (original.colour[channel] != current.colour[channel]) {
            return true;
        }
    }
    if (original.diffuse_scale != current.diffuse_scale
        || baked_emission(original) != baked_emission(current)) {
        return true;
    }
    const auto emission = baked_emission(current);
    const bool emits = *std::max_element(emission.begin(), emission.end()) > 0;
    if (emits
        && (original.bake_mesh_rgb.has_value() != current.bake_mesh_rgb.has_value()
            || original.bake_use_cutout != current.bake_use_cutout)) {
        return true;
    }
    return emits && !current.bake_mesh_rgb
           && (original.emission_image != current.emission_image
               || original.emission_contrast != current.emission_contrast
               || original.average_emission_for_bake != current.average_emission_for_bake
               || original.emission_vertex_colour != current.emission_vertex_colour);
}

// Fingerprints normalized authored transport fields only after they change effective static transport.
Result<std::string> transport_stamp(
    const MaterialSource& source,
    const Material& original,
    const MaterialBinding& binding,
    const MaterialAssets& assets
) {
    if (!source.transport || original.metal || original.blend || original.colour[3] < .99f
        || !transport_changed(original, binding.surface)) {
        return std::string{};
    }
    materials::Catalog projected;
    auto properties = materials::transport_properties(binding.values.properties);
    if (binding.surface.bake_mesh_rgb) {
        for (const auto* key :
             {"emission.runtime_intensity",
              "emission.bake_intensity",
              "emission.texture",
              "emission.use_base_texture",
              "emission.contrast",
              "emission.vertex_colour",
              "emission.average_for_bake"}) {
            properties.erase(key);
        }
    } else {
        properties.erase("emission.bake_mesh_rgb");
        if (materials::text_value(properties, "emission.bake_mode") == "texture") {
            properties.erase("emission.average_for_bake");
        }
        // A bake override inherited from a legacy profile also decouples visible emission changes.
        if (binding.surface.bake_intensity) {
            properties.erase("emission.runtime_intensity");
        }
    }
    projected.presets.emplace("transport", std::move(properties));
    const auto text = materials::catalog_text(projected);
    if (!text) {
        return unexpected(text.error());
    }
    std::string stamp = source.model_name + ":" + std::to_string(source.material) + ":" + *text;
    // A referenced mod texture can change transport without changing its authored filename.
    if (!binding.surface.bake_mesh_rgb && binding.values.properties.contains("emission.texture")) {
        const auto image = assets.image_hashes.find(binding.surface.emission_image);
        if (image != assets.image_hashes.end()) {
            stamp += image->second;
        }
    }
    return stamp;
}
}  // namespace

Result<std::shared_ptr<const MaterialBindings>> resolve_materials(
    std::shared_ptr<const MaterialAssets> assets, const materials::Catalog& catalog
) {
    try {
        if (!assets) {
            return unexpected(Error{"material identities are unavailable"});
        }
        auto result = std::make_shared<MaterialBindings>();
        result->assets = std::move(assets);
        result->catalog = catalog;
        std::map<std::pair<int, materials::Properties>, std::uint64_t> variants;
        for (const auto& source : result->assets->sources) {
            const auto properties = materials::resolve(catalog, source.identity);
            if (!properties) {
                return unexpected(properties.error());
            }
            const auto family = materials::text_value(properties->properties, "shading");
            if ((family == "water" && source.identity.material_name != "@water")
                || ((family == "chrome" || family == "glass" || family == "stylised")
                    && !source.identity.metal)) {
                return unexpected(
                    Error{
                        "material shading family requires its original water or reflective geometry"
                    }
                );
            }
            const auto coverage = materials::text_value(properties->properties, "fur.profile");
            if (!coverage.empty() && coverage != "ratchet-original") {
                return unexpected(Error{"unknown verified fur coverage profile"});
            }
            const auto& original =
                result->assets->originals.at(static_cast<std::size_t>(source.material));
            if (family == "foliage" && (original.metal || original.blend)) {
                return unexpected(
                    Error{"foliage shading requires an original opaque or cutout colour surface"}
                );
            }
            const auto material = surface(original, properties->properties, *result->assets);
            if (!material) {
                return unexpected(material.error());
            }
            const auto key = std::pair(source.material, properties->properties);
            const auto [variant, inserted] = variants.try_emplace(key, variants.size() + 1);
            MaterialBinding binding{*material, *properties, variant->second};
            const auto stamp = transport_stamp(source, original, binding, *result->assets);
            if (!stamp) {
                return unexpected(stamp.error());
            }
            result->transport_stamp += *stamp;
            result->lookup
                .emplace(std::pair(source.model, source.material), result->bindings.size());
            result->bindings.push_back(std::move(binding));
        }
        return std::shared_ptr<const MaterialBindings>(std::move(result));
    } catch (const std::exception& error) {
        return unexpected(Error{std::string("cannot resolve material bindings: ") + error.what()});
    }
}
}  // namespace openrac::scene
