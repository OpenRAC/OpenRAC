// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Convert texture emission into per-material mesh radiance.
#include "enhanced/tools/baker/sources/emission_migration.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "common/log.h"
#include "enhanced/lighting/bake_material.h"
#include "enhanced/lighting/bake_math.h"
#include "enhanced/tools/baker/sources/level_scene.h"
#include "enhanced/tools/baker/sources/scene_geometry.h"
#include "scene/material_overrides.h"

namespace openrac::lighting {
namespace {
struct MeshEnergy {
    std::array<double, 3> energy{};
    double full_area = 0;
    double covered_area = 0;
};

bool fully_opaque(const BakeTexture& texture) {
    for (std::size_t alpha = 3; alpha < texture.rgba.size(); alpha += 4) {
        if (texture.rgba[alpha] != 255) {
            return false;
        }
    }
    return true;
}

using MeshEnergies = std::map<TriangleMaterial, MeshEnergy>;

// Resolve an empty catalog too so each static material has a stable identity.
void prepare_bindings(scene::LevelData& level) {
    if (level.material_bindings) {
        return;
    }

    const materials::Catalog empty;
    const auto assets = scene::identify_materials(level, empty);
    if (!assets) {
        throw std::runtime_error(assets.error().message);
    }
    const auto bindings = scene::resolve_materials(*assets, empty);
    if (!bindings) {
        throw std::runtime_error(bindings.error().message);
    }
    level.material_bindings = *bindings;
}

scene::LevelData load_emission_level(const std::filesystem::path& level_dir) {
    scene::LevelData level;
    std::string error;
    if (!scene::load_level(level_dir, level, error)) {
        throw std::runtime_error(error);
    }
    if (!level.material_error.empty()) {
        throw std::runtime_error(level.material_error);
    }
    prepare_bindings(level);
    return level;
}

void remove_mesh_emission(materials::Properties& properties) {
    if (materials::text_value(properties, "emission.bake_mode") == "mesh") {
        properties.erase("emission.bake_mode");
    }
    properties.erase("emission.bake_mesh_rgb");
}

// Resolve the source emission again without using the previously calculated values.
void restore_source_emission(scene::LevelData& level) {
    auto catalog = level.material_bindings->catalog;
    for (auto& [id, properties] : catalog.presets) {
        remove_mesh_emission(properties);
    }
    for (auto& [id, rule] : catalog.rules) {
        remove_mesh_emission(rule.properties);
    }
    const auto bindings = scene::resolve_materials(level.material_bindings->assets, catalog);
    if (!bindings) {
        throw std::runtime_error(bindings.error().message);
    }
    level.material_bindings = *bindings;
}

// Measure original light over the full and visible areas of each material.
MeshEnergies measure_emission(const scene::LevelData& level) {
    const auto& bindings = *level.material_bindings;
    BakeSceneExport geometry;
    std::vector<TriangleMaterial> identities;
    append_probe_geometry(level, geometry, &identities);
    std::vector<bool> opaque;
    opaque.reserve(geometry.scene.textures.size());
    for (const auto& texture : geometry.scene.textures) {
        opaque.push_back(fully_opaque(texture));
    }
    MeshEnergies sums;
    std::vector<bake_detail::EmissionIntegralRequest> requests;
    for (std::size_t i = 0; i < identities.size(); ++i) {
        const auto& triangle = geometry.scene.triangles[i];
        if (triangle.mesh_emission) {
            continue;
        }
        if (triangle.emission == Float3{}) {
            const auto& [model, material] = identities[i];
            const auto* binding = bindings.find(model, material);
            if (binding
                && (binding->values.properties.contains("emission.bake_intensity")
                    || binding->values.properties.contains("emission.runtime_intensity"))) {
                sums.try_emplace(identities[i]);
            }
            continue;
        }
        const bool known_opaque = triangle.texture >= 0 && triangle.alpha_cutoff <= .5f
                                  && opaque.at(static_cast<std::size_t>(triangle.texture))
                                  && std::all_of(
                                      triangle.multipliers.begin(),
                                      triangle.multipliers.end(),
                                      [](const auto& tint) { return tint[3] == 1; }
                                  );
        requests.push_back({i, known_opaque});
    }
    const auto integrals = bake_detail::integrate_mesh_emission(geometry.scene, requests);
    for (std::size_t i = 0; i < requests.size(); ++i) {
        const auto index = requests[i].triangle;
        const auto& [a, b, c] = geometry.scene.triangles[index].vertices;
        const auto normal = bake_detail::cross(
            bake_detail::add(b, bake_detail::mul(a, -1)),
            bake_detail::add(c, bake_detail::mul(a, -1))
        );
        const double area = .5 * std::sqrt(bake_detail::dot(normal, normal));
        const auto& integrated = integrals[i];
        auto& sum = sums[identities[index]];
        const auto covered = area * integrated.coverage;
        sum.full_area += area;
        sum.covered_area += covered;
        for (std::size_t channel = 0; channel < sum.energy.size(); ++channel) {
            sum.energy[channel] += covered * integrated.average[channel];
        }
    }

    return sums;
}

// Record the measured radiance against the same model and material slot.
std::pair<std::size_t, std::size_t> add_mesh_rules(
    const scene::MaterialBindings& bindings, const MeshEnergies& sums, materials::Catalog& catalog
) {
    std::size_t migrated = 0, with_holes = 0;
    for (const auto& source : bindings.assets->sources) {
        const auto found = sums.find({source.model, source.material});
        if (found == sums.end()) {
            continue;
        }
        const auto& sum = found->second;
        const auto* binding = bindings.find(source.model, source.material);
        const bool use_cutout =
            materials::bool_value(binding->values.properties, "emission.bake_use_cutout", false);
        const double area = use_cutout ? sum.covered_area : sum.full_area;
        std::array<double, 3> rgb{};
        if (area > 0) {
            for (std::size_t channel = 0; channel < rgb.size(); ++channel) {
                rgb[channel] = sum.energy[channel] / area;
            }
        }
        materials::Rule rule;
        const auto& identity = source.identity;
        rule.id = "mesh-emission-" + identity.model + "-" + std::to_string(*identity.material_slot);
        rule.match.model = identity.model;
        rule.match.material_slot = identity.material_slot;
        rule.match.texture_path = identity.texture_path;
        rule.match.pass = "colour";
        rule.properties.emplace("emission.bake_mode", std::string("mesh"));
        rule.properties.emplace("emission.bake_mesh_rgb", rgb);
        if (use_cutout) {
            rule.properties.emplace("emission.bake_use_cutout", true);
        }
        const auto id = rule.id;
        if (!catalog.rules.emplace(id, std::move(rule)).second) {
            throw std::runtime_error("migration would replace existing rule " + id);
        }
        ++migrated;
        with_holes += sum.covered_area < sum.full_area ? 1 : 0;
    }
    if (migrated != sums.size()) {
        throw std::runtime_error("cannot identify every emitting mesh/material binding");
    }
    return {migrated, with_holes};
}
}  // namespace

std::string calculate_mesh_emission(const std::filesystem::path& level_dir) {
    auto level = load_emission_level(level_dir);
    restore_source_emission(level);
    materials::Catalog output;
    add_mesh_rules(*level.material_bindings, measure_emission(level), output);
    auto text = materials::catalog_text(output);
    if (!text) {
        throw std::runtime_error(text.error().message);
    }
    return std::move(*text);
}

void migrate_mesh_emission(
    const std::filesystem::path& level_dir, const std::filesystem::path& catalog_path
) {
    const auto level = load_emission_level(level_dir);
    const auto sums = measure_emission(level);
    auto output = materials::read_catalog(catalog_path);
    if (!output) {
        throw std::runtime_error(output.error().message);
    }
    const auto [migrated, with_holes] = add_mesh_rules(*level.material_bindings, sums, *output);
    if (migrated != 0) {
        const auto written = materials::write_catalog(catalog_path, *output);
        if (!written) {
            throw std::runtime_error(written.error().message);
        }
    }
    log::info(
        "Migrated {} mesh/material bindings ({} with partial coverage)", migrated, with_holes
    );
}
}  // namespace openrac::lighting
