// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Per-object additive lights, shared by the authoring viewer and gameplay.
#pragma once
#include <filesystem>
#include <map>
#include <memory>

#include "enhanced/renderer/local_lights.h"
#include "scene/level.h"

namespace openrac::enhanced {
// Offset and direction use the selected object/bone's local axes; range is in game/world units.
struct AttachedLight {
    std::string name = "Light", bone;
    LocalLight light;
    bool enabled = true;
    bool operator==(const AttachedLight&) const = default;
};

// A model key is layer/model-name, shared across placements; terrain additionally names its level.
struct ObjectLightCatalog {
    std::map<std::string, std::vector<AttachedLight>, std::less<>> objects;
};

// Stable authored identity, independent of live addresses, visibility and exported model indices.
std::string object_light_key(const scene::LevelData&, scene::Layer, unsigned model);
// Canonical source profile in a checkout; shared export-root override in standalone builds.
std::filesystem::path object_light_author_path(const scene::LevelData&);
// Strict versioned parser; missing files are an empty catalog, unreadable/invalid files are errors.
Result<ObjectLightCatalog> read_object_lights(const std::filesystem::path&);
// Writes a complete catalog atomically, preserving the previous file on every failed publication.
Result<void> write_object_lights(const std::filesystem::path&, const ObjectLightCatalog&);
// UTF-8 display form keeps profile diagnostics and viewer metadata independent of Windows code pages.
std::string object_light_path_text(const std::filesystem::path&);
// Merge exported defaults, developer source and mod overrides in the same precedence as geometry.
Result<ObjectLightCatalog> load_object_lights(const scene::LevelData&);
// A single immutable snapshot crosses the UI/render/capture boundaries.
void publish_object_lights(std::string game, ObjectLightCatalog catalog);
std::shared_ptr<const ObjectLightCatalog> current_object_lights(std::string_view game);
// Monotonic authoring revision; normal playback performs no per-frame file I/O.
std::uint64_t object_lights_revision();
// Bone lookup validates the original exported name; empty name attaches to the object root.
int attached_light_joint(const AttachedLight&, std::span<const scene::ModelJoint>);
// Converts object/bone-local light data through P = (P*S)*inverse(S), preserving world-space range.
Result<LocalLight> resolve_attached_light(
    const AttachedLight&,
    const scene::Instance&,
    std::span<const renderer::Mat4> palette,
    std::span<const scene::ModelJoint> bones
);

// Filters model-bound instances once when handed to the scene; only live light transforms update per frame.
class ObjectLights {
public:
    // Load shared definitions and own the static attachment metadata until scene release.
    void load(const scene::LevelData&);
    // Replace only the bound dynamic instances on the render thread.
    void instances(scene::Layer, std::span<const scene::Instance>);
    // Resolve once per displayed frame; repeated depth/colour passes reuse the records.
    std::span<const LocalLight> prepare(
        std::uint64_t frame, std::span<const renderer::Mat4> palette
    );
    // Clear level-owned data without retaining game pointers or GPU resources.
    void release();

private:
    // Rebuild model bindings after an explicit Save/Reload, without touching the game scene.
    void refresh();

    struct Binding {
        std::vector<scene::ModelJoint> bones;
        std::vector<AttachedLight> lights;
    };

    // Resolve one instance without making an unknown bone affect unrelated emitters.
    bool append_instance(
        const scene::Instance&, const Binding&, std::span<const renderer::Mat4>, std::string& error
    );
    // Deduplicate persistent profile errors.
    void report_error(std::string error);
    std::string m_game, m_error;
    std::array<std::map<unsigned, Binding>, scene::kLayerCount> m_bindings;
    std::array<std::vector<scene::Instance>, scene::kLayerCount> m_instances;
    std::array<std::vector<scene::Instance>, scene::kLayerCount> m_static;
    std::array<
        std::map<unsigned, std::pair<std::string, std::vector<scene::ModelJoint>>>,
        scene::kLayerCount>
        m_models;
    std::vector<LocalLight> m_resolved;
    std::uint64_t m_revision = ~std::uint64_t{0}, m_frame = ~std::uint64_t{0};
};
}  // namespace openrac::enhanced
