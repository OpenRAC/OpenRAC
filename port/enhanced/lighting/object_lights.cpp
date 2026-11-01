// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Canonical profile selection, immutable editor snapshots and bone-space light resolution.
#include "enhanced/lighting/object_lights.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <mutex>

#include "common/env.h"
#include "common/stats.h"

namespace openrac::enhanced {
namespace {
// The registry owns immutable snapshots; readers keep them alive independently of Save or level unload.
struct CatalogState {
    std::mutex mutex;
    std::map<std::string, std::shared_ptr<const ObjectLightCatalog>, std::less<>> catalogs;
    std::atomic<std::uint64_t> revision{0};
};

CatalogState& catalog_state() {
    static CatalogState state;
    return state;
}

// Never derive an authoring path from traversal, separator or drive syntax in an external manifest.
bool safe_game(std::string_view game) {
    return !game.empty() && std::all_of(game.begin(), game.end(), [](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
               || c == '_' || c == '-';
    });
}

// Source profiles are preferred only in developer mode; packaged players use exported data alone.
std::filesystem::path source_path(const scene::LevelData& level) {
    const auto root = std::filesystem::path(stats::shader_root()) / "enhanced/lighting/profiles";
    std::error_code error;

    if (!safe_game(level.game) || !env::flag(env::Var::Developer)
        || !std::filesystem::is_directory(root, error)) {
        return {};
    }

    return root / level.game / "object-lights.json";
}
}  // namespace

std::string object_light_key(const scene::LevelData& level, scene::Layer layer, unsigned model) {
    if (model >= level.models.size() || unsigned(layer) >= scene::kLayerCount) {
        return {};
    }

    std::string prefix = scene::layer_name(layer);

    // Terrain is level geometry, rather than a shared prop class with reusable placements.
    if (layer == scene::Layer::Terrain) {
        prefix = "level_" + std::to_string(level.level) + "/" + prefix;
    }

    return prefix + "/" + level.models[model].name;
}

std::filesystem::path object_light_author_path(const scene::LevelData& level) {
    if (!level.overrides.empty()) {
        return level.overrides / "object-lights.json";
    }

    auto source = source_path(level);

    if (!source.empty()) {
        return source;
    }

    return level.dir.parent_path() / "object-lights.json";
}

Result<ObjectLightCatalog> load_object_lights(const scene::LevelData& level) {
    std::vector<std::filesystem::path> paths{
        level.dir / "object-lights.json", level.dir.parent_path() / "object-lights.json"
    };
    auto source = source_path(level);

    if (!source.empty()) {
        paths.push_back(source);
    }

    if (!level.overrides.empty()) {
        paths.push_back(level.overrides / "object-lights.json");
    }

    // The first mod root has highest priority, as in the geometry and texture loaders.
    for (auto at = level.mod_roots.rbegin(); at != level.mod_roots.rend(); ++at) {
        paths.push_back(*at / "object-lights.json");
    }

    ObjectLightCatalog merged;

    for (const auto& path : paths) {
        auto catalog = read_object_lights(path);

        if (!catalog) {
            return unexpected(catalog.error());
        }

        for (auto& [key, lights] : catalog.value().objects) {
            merged.objects[key] = std::move(lights);
        }
    }

    return merged;
}

void publish_object_lights(std::string game, ObjectLightCatalog catalog) {
    auto snapshot = std::make_shared<const ObjectLightCatalog>(std::move(catalog));
    auto& state = catalog_state();
    std::lock_guard lock(state.mutex);
    state.catalogs[std::move(game)] = std::move(snapshot);
    ++state.revision;
}

std::shared_ptr<const ObjectLightCatalog> current_object_lights(std::string_view game) {
    auto& state = catalog_state();
    std::lock_guard lock(state.mutex);
    const auto found = state.catalogs.find(game);
    return found == state.catalogs.end() ? nullptr : found->second;
}

std::uint64_t object_lights_revision() {
    return catalog_state().revision.load();
}

int attached_light_joint(const AttachedLight& light, std::span<const scene::ModelJoint> bones) {
    if (light.bone.empty()) {
        return -1;
    }

    for (std::size_t j = 0; j < bones.size(); ++j) {
        if (bones[j].name == light.bone) {
            return int(j);
        }
    }

    return -2;  // Distinguish an unknown bone from an intentional root attachment.
}

Result<LocalLight> resolve_attached_light(
    const AttachedLight& light,
    const scene::Instance& instance,
    std::span<const renderer::Mat4> palette,
    std::span<const scene::ModelJoint> bones
) {
    auto frame = instance.matrix;
    const int joint = attached_light_joint(light, bones);

    if (joint == -2) {
        return unexpected(Error{"unknown light attachment bone: " + light.bone});
    }

    if (joint >= 0) {
        auto pose = bones[std::size_t(joint)].bind;

        if (instance.palette >= 0) {
            const auto index = std::size_t(instance.palette) + std::size_t(joint);

            if (index >= palette.size()) {
                return unexpected(Error{"light attachment has an incomplete joint palette"});
            }

            pose = renderer::multiply(palette[index], pose);
        }

        frame = renderer::multiply(frame, pose);
    }

    auto resolved = light.light;
    resolved.position = renderer::transform_point(frame, resolved.position);
    resolved.direction = renderer::normalize(renderer::transform_vector(frame, resolved.direction));

    for (unsigned c = 0; c < 3; ++c) {
        if (!std::isfinite(resolved.position[c]) || !std::isfinite(resolved.direction[c])) {
            return unexpected(Error{"light attachment transform is not finite"});
        }
    }

    // A collapsed spot axis cannot be normalized into a meaningful cone.
    if (resolved.spot && renderer::dot(resolved.direction, resolved.direction) < .5f) {
        return unexpected(Error{"light attachment has a singular direction transform"});
    }

    return resolved;
}
}  // namespace openrac::enhanced
