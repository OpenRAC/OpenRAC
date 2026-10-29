// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "custom_map.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

#include "common/json.h"

namespace openrac::program {

namespace {
// Validate a map record without changing the caller's output on failure.
bool read_map(
    const json::Value& root,
    std::string_view game,
    std::string_view version,
    CustomMap& out,
    std::string& error
) {
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

    for (auto [key, value] :
         {std::pair{"skip_movies", &map.skip_movies},
          std::pair{"skip_cutscenes", &map.skip_cutscenes}}) {
        if (!root[key].is_null()) {
            if (root[key].type() != json::Value::Type::Bool) {
                error = std::string("map ") + key + " must be true or false";
                return false;
            }

            *value = root[key].boolean();
        }
    }

    out = std::move(map);
    return true;
}

}  // namespace

bool parse_custom_map(
    std::string_view text,
    std::string_view game,
    std::string_view version,
    CustomMap& out,
    std::string& error
) {
    json::Value root;
    return json::parse(text, root, error) && read_map(root, game, version, out, error);
}

bool parse_destinations(
    std::string_view text,
    std::string_view game,
    std::string_view version,
    std::vector<CustomMap>& out,
    std::string& error
) {
    json::Value root;
    if (!json::parse(text, root, error)) {
        return false;
    }
    const auto& entries = root["destinations"];
    if (root["format"].number() != 1 || root["game"].string() != game || game != "rac1"
        || root["version"].string() != version || version != "pal" || !entries.is_array()
        || entries.items().empty() || entries.items().size() > 256) {
        error = "destinations.json must contain 1 to 256 destinations for RAC1 PAL";
        return false;
    }
    std::vector<CustomMap> maps;
    for (const auto& entry : entries.items()) {
        CustomMap map;
        if (!read_map(entry, game, version, map, error)) {
            return false;
        }
        // IDs also name the stack's private subdirectories: never accept paths here.
        if (map.id.empty() || map.id == "." || map.id == ".."
            || std::any_of(
                map.id.begin(),
                map.id.end(),
                [](unsigned char c) {
                    return !(
                        (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_'
                    );
                }
            )
            || std::any_of(maps.begin(), maps.end(), [&](const auto& old) {
                   return old.id == map.id;
               })) {
            error =
                "destination IDs must be unique folder IDs using lowercase letters, digits, - or _";
            return false;
        }
        maps.push_back(std::move(map));
    }
    out = std::move(maps);
    return true;
}

}  // namespace openrac::program
