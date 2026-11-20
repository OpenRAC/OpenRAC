// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "scene/moby_skin.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <map>

#include "common/json.h"

namespace openrac::scene {

namespace fs = std::filesystem;

namespace {

// `relative` inside `root`, or empty.
fs::path inside(const fs::path& root, const std::string& relative) {
    const fs::path path(relative);
    if (relative.empty() || path.is_absolute() || path.has_root_name()) {
        return {};
    }
    std::error_code error;
    const fs::path resolved = fs::weakly_canonical(root / path, error);
    if (error) {
        return {};
    }
    const fs::path contained = resolved.lexically_relative(root);
    if (contained.empty() || contained.is_absolute()
        || std::any_of(contained.begin(), contained.end(), [](const fs::path& part) {
               return part == "..";
           })) {
        return {};
    }
    return resolved;
}

}  // namespace

bool load_moby_skins(
    const fs::path& file,
    LevelData& level,
    MobySkins& out,
    std::string& error,
    std::vector<std::string>& warnings
) {
    json::Value doc;
    if (!json::parse_file(file.string(), doc, error)) {
        return false;
    }
    if (doc["format"].number(-1) != 1 || !doc["skins"].is_array()) {
        error = "skins.json must use format 1 and have a skins array";
        return false;
    }
    std::error_code fs_error;
    const fs::path root = fs::canonical(file.parent_path(), fs_error);
    if (fs_error) {
        error = "cannot resolve the folder of skins.json";
        return false;
    }
    // Each model once, however many mobys wear it.
    std::map<fs::path, std::pair<std::uint32_t, PropRig>> models;
    for (const json::Value& entry : doc["skins"].items()) {
        const double spawn_id = entry["spawn_id"].number(-1);
        const double scale = entry["scale"].number(1.0);
        if (!entry.is_object() || !(spawn_id >= 0 && spawn_id < 0xFFFF)
            || spawn_id != std::floor(spawn_id) || !(scale > 0.0 && scale < 1000.0)) {
            error = "a skin needs a spawn_id (0 to 65534) and a scale above 0";
            return false;
        }
        const std::string& name = entry["model"].string();
        const fs::path path = inside(root, name);
        if (path.empty()) {
            error = std::format("skin model {} must be a relative path inside the pack", name);
            return false;
        }
        auto found = models.find(path);
        if (found == models.end()) {
            std::string why;
            auto rig = load_prop_rig(path, why);
            if (!rig || rig->animations.empty()) {
                warnings.push_back(
                    std::format("skin {}: {}", name, rig ? "the model has no animations" : why)
                );
                continue;
            }
            const auto model = load_model(path, level, why);
            if (!model) {
                warnings.push_back(std::format("skin {}: {}", name, why));
                continue;
            }
            found = models.emplace(path, std::make_pair(*model, std::move(*rig))).first;
        }
        MobySkin skin;
        skin.model = found->second.first;
        skin.rig = found->second.second;
        skin.scale = static_cast<float>(scale);
        if (entry["animations"].is_array()) {
            for (const json::Value& number : entry["animations"].items()) {
                skin.animations.push_back(static_cast<int>(number.number(-1)));
            }
        }
        out.by_spawn_id[static_cast<int>(spawn_id)] = std::move(skin);
    }
    return true;
}

int moby_skin_animation(const MobySkin& skin, int sequence) {
    const int count = static_cast<int>(skin.rig.animations.size());
    if (sequence >= 0 && static_cast<std::size_t>(sequence) < skin.animations.size()) {
        const int chosen = skin.animations[static_cast<std::size_t>(sequence)];
        if (chosen >= 0 && chosen < count) {
            return chosen;
        }
    }
    return sequence >= 0 && sequence < count ? sequence : 0;
}

float moby_skin_seconds(MobySkins& skins, std::uint32_t address, int sequence) {
    MobySkins::Clock& clock = skins.clocks[address];
    if (clock.sequence != sequence) {
        clock = {sequence, 0};
    } else {
        ++clock.ticks;
    }
    return static_cast<float>(clock.ticks) / 60.0f;
}

}  // namespace openrac::scene
