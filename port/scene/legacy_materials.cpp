// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Read-only migration adapter for independent emission edits in existing level exports.
#include <algorithm>
#include <array>

#include "common/json.h"
#include "scene/material_overrides.h"

namespace openrac::scene {
namespace {
// Generated settings compare by value rather than JSON formatting.
bool same_value(const json::Value& first, const json::Value& second) {
    if (first.type() != second.type()) {
        return false;
    }
    if (first.is_number()) {
        return first.number() == second.number();
    }
    if (first.is_string()) {
        return first.string() == second.string();
    }
    return first.type() == json::Value::Type::Bool && first.boolean() == second.boolean();
}

// Converts authored fields only; static eligibility and surfaces remain separate bake inputs.
materials::Properties authored_properties(
    const json::Value& current, const json::Value& generated
) {
    materials::Properties result;
    constexpr std::string_view kNumbers[] =
        {"runtime_intensity", "bake_intensity", "contrast", "diffuse_scale"};
    for (const auto& [key, value] : current.members()) {
        if (same_value(value, generated[key])) {
            continue;
        }
        const std::string path =
            key == "diffuse_scale" ? "surface.diffuse_scale" : "emission." + key;
        if (std::find(std::begin(kNumbers), std::end(kNumbers), key) != std::end(kNumbers)) {
            result[path] = value.number(-1);
        } else if (key == "vertex_colour" || key == "use_base_texture"
                   || key == "average_for_bake") {
            result[path] = value.boolean();
        } else if (key == "texture") {
            result[path] = value.string();
        }
    }
    return result;
}
}  // namespace

Result<materials::Catalog> legacy_material_edits(const LevelData& level) {
    auto roots = level.mod_roots;
    if (!level.overrides.empty()) {
        roots.push_back(level.overrides);
    }
    roots.push_back(level.dir);
    std::filesystem::path path;
    for (const auto& root : roots) {
        std::error_code error;
        const auto candidate = root / "probe-emission.json";
        if (std::filesystem::is_regular_file(candidate, error)) {
            path = candidate;
            break;
        }
        if (error && error != std::errc::no_such_file_or_directory) {
            return unexpected(Error{error.message()});
        }
    }
    if (path.empty()) {
        return materials::Catalog{};
    }
    json::Value document;
    std::string error;
    if (!json::parse_file(path.string(), document, error)) {
        return unexpected(Error{error});
    }
    materials::Catalog result;
    for (const auto& [texture, current] : document.members()) {
        if (!texture.starts_with("textures/") || !current.is_object()) {
            continue;
        }
        auto properties = authored_properties(current, document["_catalog_defaults"][texture]);
        if (properties.empty()) {
            continue;
        }
        const auto valid = materials::validate(properties);
        if (!valid) {
            return unexpected(valid.error());
        }
        materials::Rule rule;
        rule.id = "legacy-emission-" + std::to_string(result.rules.size());
        rule.match.texture_path = texture;
        rule.properties = std::move(properties);
        rule.source = path.string();
        result.rules.emplace(rule.id, std::move(rule));
    }
    return result;
}
}  // namespace openrac::scene
