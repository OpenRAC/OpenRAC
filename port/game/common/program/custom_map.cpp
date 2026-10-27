// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "custom_map.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

#include "common/json.h"

namespace openrac::program {

bool parse_custom_map(
    std::string_view text,
    std::string_view game,
    std::string_view version,
    CustomMap& out,
    std::string& error
) {
    json::Value root;

    if (!json::parse(text, root, error)) {
        return false;
    }

    if (!root.is_object() || !root["format"].is_number() || root["format"].number() != 1
        || !root["game"].is_string() || root["game"].string() != game || game != "rac1"
        || !root["version"].is_string() || root["version"].string() != version) {
        error = "map.json must be format 1 for this game and disc";
        return false;
    }

    CustomMap map;

    for (auto [key, value] : {std::pair{"id", &map.id}, std::pair{"name", &map.name}}) {
        if (!root[key].is_string() || root[key].string().empty() || root[key].string().size() > 120
            || std::any_of(
                root[key].string().begin(), root[key].string().end(), [](unsigned char c) {
                    return c < 32;
                }
            )) {
            error = "map.json needs a non-empty id and name of up to 120 bytes";
            return false;
        }

        *value = root[key].string();
    }

    const auto& level = root["level"];

    if (!level.is_number() || !std::isfinite(level.number()) || level.number() < 0
        || level.number() >= 19 || std::floor(level.number()) != level.number()) {
        error = "map level must be an integer from 0 to 18";
        return false;
    }

    map.level = static_cast<int>(level.number());
    const auto& spawn = root["spawn"];

    if (!spawn.is_array() || spawn.items().size() != 3) {
        error = "map spawn must contain three finite coordinates";
        return false;
    }

    auto number = [](const json::Value& value) {
        return value.is_number() && std::isfinite(value.number())
               && std::fabs(value.number()) <= std::numeric_limits<float>::max();
    };

    for (std::size_t i = 0; i < map.spawn.size(); ++i) {
        if (!number(spawn.items()[i])) {
            error = "map spawn must contain three finite coordinates";
            return false;
        }

        map.spawn[i] = static_cast<float>(spawn.items()[i].number());
    }

    if (!root["heading"].is_null()) {
        if (!number(root["heading"])) {
            error = "map heading must be a finite angle in radians";
            return false;
        }

        map.heading = static_cast<float>(root["heading"].number());
    }

    if (!root["skip_movies"].is_null()) {
        if (root["skip_movies"].type() != json::Value::Type::Bool) {
            error = "map skip_movies must be true or false";
            return false;
        }

        map.skip_movies = root["skip_movies"].boolean();
    }

    out = std::move(map);
    return true;
}

}  // namespace openrac::program
