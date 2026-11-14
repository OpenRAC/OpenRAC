// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Prepares linked retail chrome coverage and Lit replacement geometry for level.h.
#include <algorithm>
#include <cmath>
#include <map>
#include <optional>
#include <span>

#include "scene/level.h"

namespace openrac::scene {
namespace {
// Coincident corners must share skinning before their surfaces can use one material choice.
using ChromeCorner = std::pair<Vec3, std::array<std::pair<std::uint8_t, float>, 4>>;
using ChromeFace = std::array<ChromeCorner, 3>;
using ColourFaces = std::map<ChromeFace, int>;
using ChromeCoverage = std::map<int, std::vector<std::uint32_t>>;

// Canonicalises winding and influence order without merging corners that animate differently.
std::optional<ChromeFace> chrome_face(const LevelData& level, std::size_t offset) {
    ChromeFace face;
    for (std::size_t corner = 0; corner < face.size(); ++corner) {
        const auto index = level.indices[offset + corner];
        if (index >= level.vertices.size()) {
            return std::nullopt;
        }
        const auto& vertex = level.vertices[index];
        auto& [position, influences] = face[corner];
        std::copy_n(vertex.position, 3, position.begin());
        if (!std::ranges::all_of(position, [](float value) { return std::isfinite(value); })) {
            return std::nullopt;
        }
        for (std::size_t joint = 0; joint < influences.size(); ++joint) {
            const float weight = vertex.weights[joint];
            influences[joint] = weight > 0.f ? std::pair{vertex.joints[joint], weight}
                                             : std::pair<std::uint8_t, float>{};
        }
        std::ranges::sort(influences);
    }
    std::sort(face.begin(), face.end());
    return face;
}

// Maps existing colour faces to the editable material that owns their chrome overlay.
ColourFaces colour_faces(const LevelData& level, const Model& model) {
    ColourFaces result;
    for (const auto& primitive : model.primitives) {
        if (level.materials[primitive.material].metal) {
            continue;
        }
        const auto end = std::min(
            std::size_t(primitive.first_index) + primitive.index_count, level.indices.size()
        );
        for (std::size_t offset = primitive.first_index; offset + 2 < end; offset += 3) {
            if (const auto face = chrome_face(level, offset)) {
                result.try_emplace(*face, primitive.material);
            }
        }
    }
    return result;
}

// Partitions a retail packet by editable colour slot; -1 denotes faces needing Lit replacements.
ChromeCoverage chrome_coverage(
    const LevelData& level, const Primitive& primitive, const ColourFaces& colours
) {
    ChromeCoverage result;
    const auto end =
        std::min(std::size_t(primitive.first_index) + primitive.index_count, level.indices.size());
    for (std::size_t offset = primitive.first_index; offset + 2 < end; offset += 3) {
        const auto face = chrome_face(level, offset);
        if (!face) {
            continue;
        }
        const auto colour = colours.find(*face);
        auto& indices = result[colour == colours.end() ? -1 : colour->second];
        indices.insert(
            indices.end(), level.indices.begin() + offset, level.indices.begin() + offset + 3
        );
    }
    return result;
}

// Copies a range before growing the source index array, retaining every primitive packet flag.
Primitive indexed_primitive(
    LevelData& level, Primitive primitive, std::span<const std::uint32_t> indices
) {
    primitive.first_index = static_cast<std::uint32_t>(level.indices.size());
    primitive.index_count = static_cast<std::uint32_t>(indices.size());
    level.indices.insert(level.indices.end(), indices.begin(), indices.end());
    return primitive;
}

// Reuses one untextured colour material per retail chrome map across the level's models.
int replacement_material(LevelData& level, int original, std::map<int, int>& replacements) {
    const auto [material, inserted] =
        replacements.try_emplace(original, static_cast<int>(level.materials.size()));
    if (inserted) {
        auto standard = level.materials[original];
        standard.metal = false;
        standard.cutout = false;
        standard.blend = false;
        // Sphere-map UVs cannot serve as albedo coordinates.
        standard.image = -1;
        level.materials.push_back(std::move(standard));
    }
    return material->second;
}

// Keeps retail packet coverage linked to its colour surface or its generated replacement.
void prepare_model(LevelData& level, Model& model, std::map<int, int>& materials) {
    const auto colours = colour_faces(level, model);
    std::vector<Primitive> prepared, replacements;
    for (const auto& primitive : model.primitives) {
        const auto& material = level.materials[primitive.material];
        if (!is_retail_chrome(material.metal, material.name)) {
            prepared.push_back(primitive);
            continue;
        }
        const auto coverage = chrome_coverage(level, primitive, colours);
        int control_material = -1;
        if (const auto unmatched = coverage.find(-1); unmatched != coverage.end()) {
            auto replacement = indexed_primitive(level, primitive, unmatched->second);
            control_material = replacement_material(level, primitive.material, materials);
            replacement.material = control_material;
            replacement.enhanced_only = true;
            replacements.push_back(replacement);
        }
        for (const auto& [control, indices] : coverage) {
            auto retail = primitive;
            retail.appearance_material = control < 0 ? control_material : control;
            if (coverage.size() > 1 || indices.size() != primitive.index_count) {
                retail = indexed_primitive(level, retail, indices);
            }
            prepared.push_back(retail);
        }
    }
    model.primitives = std::move(prepared);
    model.primitives.insert(model.primitives.end(), replacements.begin(), replacements.end());
}
}  // namespace

void prepare_chrome_geometry(LevelData& level) {
    std::map<int, int> standard_materials;
    for (auto& model : level.models) {
        const bool chrome = std::ranges::any_of(model.primitives, [&](const auto& primitive) {
            const auto& material = level.materials[primitive.material];
            return is_retail_chrome(material.metal, material.name);
        });
        if (chrome) {
            prepare_model(level, model, standard_materials);
        }
    }
}
}  // namespace openrac::scene
