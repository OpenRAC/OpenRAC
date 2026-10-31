// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Verifies authored fur profiles against a body's UV layout before selecting its surface.
#include <algorithm>
#include <bit>
#include <charconv>
#include <cmath>
#include <limits>

#include "common/json.h"
#include "enhanced/renderer/fur.h"
#include "fur_profile_data.h"

namespace openrac::enhanced {
namespace {
// Normalises an authored UV direction before it is encoded as a map.
std::array<float, 2> read_direction(const json::Value& source) {
    std::array<float, 2> flow{
        float(source["direction"][0].number()), float(source["direction"][1].number())
    };
    const float length = std::hypot(flow[0], flow[1]);
    if (length > 0) {
        flow[0] /= length;
        flow[1] /= length;
    }
    return flow;
}

// Reads the compiled, authored profile without loading or modifying any retail pixels.
FurProfile read_profile() {
    json::Value root;
    std::string error;
    if (!json::parse(kRac1FurProfile, root, error)) {
        return {};
    }
    FurProfile profile;
    profile.game = root["game"].string();
    profile.material = root["material"].string();
    profile.class_id = int(root["class_id"].number(-1));
    profile.width = int(root["size"][0].number());
    profile.height = int(root["size"][1].number());
    profile.index_count = unsigned(root["index_count"].number());
    profile.vertex_count = unsigned(root["vertex_count"].number());
    const auto& signature = root["uv_signature"].string();
    std::from_chars(
        signature.data(), signature.data() + signature.size(), profile.uv_signature, 16
    );
    for (const auto& source : root["patches"].items()) {
        FurPatch patch;
        patch.direction = read_direction(source);
        for (const auto& range : source["triangles"].items()) {
            patch.triangles.push_back({unsigned(range[0].number()), unsigned(range[1].number())});
        }
        profile.patches.push_back(std::move(patch));
    }
    for (const auto& source : root["regions"].items()) {
        FurRegion region;
        region.exclude = source["exclude"].boolean();
        region.direction = read_direction(source);
        for (const auto& point : source["polygon"].items()) {
            region.polygon.push_back({float(point[0].number()), float(point[1].number())});
        }
        profile.regions.push_back(std::move(region));
    }
    return profile;
}

// Finds the exact profiled material and its contiguous global vertex range.
std::optional<FurSurface> find_surface(
    const scene::LevelData& level, unsigned model, const FurProfile& profile
) {
    std::optional<FurSurface> result;
    for (const auto& primitive : level.models[model].primitives) {
        if (primitive.material < 0 || std::size_t(primitive.material) >= level.materials.size()) {
            return std::nullopt;
        }
        const auto& material = level.materials[std::size_t(primitive.material)];
        if (material.name != profile.material) {
            continue;
        }
        if (result || material.metal || material.blend || material.image < 0
            || std::size_t(material.image) >= level.images.size()
            || level.images[std::size_t(material.image)].width != profile.width
            || level.images[std::size_t(material.image)].height != profile.height
            || primitive.index_count != profile.index_count || !profile.uv_signature
            || fur_uv_signature(level, primitive) != profile.uv_signature) {
            return std::nullopt;
        }
        unsigned first = std::numeric_limits<unsigned>::max(), last = 0;
        for (unsigned n = 0; n < primitive.index_count; ++n) {
            const auto index = level.indices[primitive.first_index + n];
            first = std::min(first, index);
            last = std::max(last, index);
        }
        if (first > last || last - first + 1 != profile.vertex_count) {
            return std::nullopt;
        }
        result = FurSurface{
            model,
            first,
            profile.vertex_count,
            unsigned(&primitive - level.models[model].primitives.data()),
            {}
        };
    }
    return result;
}

// A material range must not include vertices referenced by any other material.
bool exclusive_material(
    const scene::LevelData& level, const FurSurface& surface, const FurProfile& profile
) {
    const unsigned model = surface.model;

    for (const auto& primitive : level.models[model].primitives) {
        if (level.materials[std::size_t(primitive.material)].name == profile.material) {
            continue;
        }
        if (primitive.first_index > level.indices.size()
            || primitive.index_count > level.indices.size() - primitive.first_index) {
            return false;
        }
        for (unsigned n = 0; n < primitive.index_count; ++n) {
            const auto index = level.indices[primitive.first_index + n];
            if (index >= surface.first && index - surface.first < surface.count) {
                return false;
            }
        }
    }

    return true;
}

// Marks one labelled triangle without changing the geometry or its indices.
void select_triangle(
    const scene::LevelData& level,
    const scene::Primitive& primitive,
    unsigned triangle,
    FurSurface& surface
) {
    for (unsigned corner = 0; corner < 3; ++corner) {
        const auto index = level.indices[primitive.first_index + triangle * 3 + corner];
        surface.vertices[index - surface.first] = 1;
    }
}

// Labels whole UV islands, refusing a topology that mixes selected and excluded vertices.
bool select_vertices(
    const scene::LevelData& level, FurSurface& surface, const FurProfile& profile
) {
    const unsigned model = surface.model;

    const auto& primitive = level.models[model].primitives[surface.primitive];
    surface.vertices.resize(surface.count);
    std::vector<bool> selected(primitive.index_count / 3);
    for (const auto& patch : profile.patches) {
        for (const auto& range : patch.triangles) {
            if (range[0] > range[1] || range[1] >= selected.size()) {
                return false;
            }
            for (unsigned triangle = range[0]; triangle <= range[1]; ++triangle) {
                selected[triangle] = true;
                select_triangle(level, primitive, triangle, surface);
            }
        }
    }
    // Flat interpolation is safe only when excluded islands share no selected vertices.
    for (unsigned n = 0; n < primitive.index_count; ++n) {
        const auto index = level.indices[primitive.first_index + n];
        if (!selected[n / 3] && surface.vertices[index - surface.first]) {
            return false;
        }
    }

    return std::any_of(surface.vertices.begin(), surface.vertices.end(), [](unsigned selected) {
        return selected != 0;
    });
}
}  // namespace

const FurProfile& rac1_fur_profile() {
    static const FurProfile profile = read_profile();
    return profile;
}

std::uint64_t fur_uv_signature(const scene::LevelData& level, const scene::Primitive& primitive) {
    if (primitive.first_index > level.indices.size()
        || primitive.index_count > level.indices.size() - primitive.first_index) {
        return 0;
    }
    // FNV-1a of little-endian IEEE UV words in triangle order; independent of global vertex IDs.
    std::uint64_t hash = 14695981039346656037ull;
    for (unsigned n = 0; n < primitive.index_count; ++n) {
        const auto index = level.indices[primitive.first_index + n];
        if (index >= level.vertices.size()) {
            return 0;
        }
        for (float uv : level.vertices[index].uv) {
            if (!std::isfinite(uv)) {
                return 0;
            }
            const auto bits = std::bit_cast<std::uint32_t>(uv);
            for (unsigned shift = 0; shift < 32; shift += 8) {
                hash = (hash ^ ((bits >> shift) & 255u)) * 1099511628211ull;
            }
        }
    }
    return hash;
}

std::optional<FurSurface> identify_fur_surface(
    const scene::LevelData& level, const FurProfile& profile
) {
    // Unknown games, replacement assets and stand-ins must not receive an atlas-specific mask.
    if (level.game != profile.game || profile.game.empty() || !level.overrides.empty()) {
        return std::nullopt;
    }
    const auto found = level.moby_classes.find(profile.class_id);
    if (found == level.moby_classes.end() || found->second.second
        || found->second.first >= level.models.size()) {
        return std::nullopt;
    }
    auto result = find_surface(level, found->second.first, profile);
    if (!result || !exclusive_material(level, *result, profile)
        || !select_vertices(level, *result, profile)) {
        return std::nullopt;
    }
    return result;
}
}  // namespace openrac::enhanced
