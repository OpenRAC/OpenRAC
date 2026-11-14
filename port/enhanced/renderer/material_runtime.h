// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Cached GPU material responses and sparse authoring edits for one enhanced scene.
#pragma once
#include <filesystem>
#include <memory>
#include <set>
#include <span>
#include <vector>

#include "enhanced/renderer/foliage.h"
#include "enhanced/renderer/fur.h"
#include "enhanced/renderer/reflection_material.h"
#include "enhanced/renderer/wetness.h"
#include "scene/material_overrides.h"

namespace openrac::enhanced {
// Straight-through transmission for a thin pane, with a linear RGB colour veil.
struct ThinGlassSettings {
    std::array<float, 3> tint{.05f, .15f, .22f};  // Linear colour of the veil.
    float transmission = .9f;                     // Fraction of clear background before Fresnel.
};

// Typed responses compiled at load/edit, never interpreted from JSON during drawing.
struct MaterialResponse {
    bool original = false;
    bool glass_surface = false;
    bool wet_fur = false;
    std::array<float, 3> sky_tint{1, 1, 1};  // RGB multiplier for the separate sky pass.
    ReflectionMaterialSettings reflection;   // Original reflection family with supplied overrides.
    ThinGlassSettings glass;                 // Thin panes consume these transmission controls.
    FurMaterialSettings fur;                 // Applied only to verified coverage by HeroFur.
    WetnessMaterialSettings wetness;         // Applied only to HeroWetness's verified vertices.
    int fallback_channels = 0;               // Bit 0 diffuse scale, bit 1 authored emission.
    bool fur_override = false;               // Explicit fur fields select cached authored response.
    bool wetness_override = false;     // Explicit wetness fields select cached authored response.
    bool reflection_override = false;  // Authored optical fields can use the retail fallback.
    bool family_override = false;      // Explicit catalog family, including standard opt-outs.
    float metallic = -1.f;  // Negative means the material still uses its original response.
    bool dielectric_specular = false;
    std::array<std::uint32_t, 2> foliage{};  // Existing packed optical controls.
};

// Immutable inspector snapshot, shared between frontend and scene while the render worker is idle.
struct MaterialEditor {
    std::shared_ptr<const scene::MaterialBindings> bindings;  // Current resolved preview.
    std::filesystem::path path;                               // Exact atomic-save target.
    std::string error;                                        // Load error, if any.
    std::size_t foliage_bindings = 0;  // Explicit thin-leaf bindings in this level.
    std::set<std::string> local_rules;  // Overrides owned by the current save file.
    bool dirty = false;                // Session edits await explicit Save.
    bool transport_dirty = false;      // Effective static lighting changed.
};

// Scene-owned cache; callers edit only after render completion, matching LevelScene's ownership handoff.
class MaterialRuntime {
public:
    // Loads prepared bindings and the separate editable overlay; no GPU operations.
    [[nodiscard]] Result<void> load(const scene::LevelData& level);
    // Clears level ownership and all unsaved edits.
    void clear();

    // Returns the immutable current inspector state.
    [[nodiscard]] std::shared_ptr<const MaterialEditor> editor() const { return m_editor; }

    // Returns cached surface/response/variant or the caller's legacy fallback.
    [[nodiscard]] const scene::MaterialBinding* find(std::uint32_t model, int material) const;
    [[nodiscard]] const MaterialResponse* response(std::uint32_t model, int material) const;
    // Applies one sparse object-type/slot property, rejecting the whole edit on validation errors.
    [[nodiscard]] Result<void> patch(std::size_t binding, std::string key, materials::Value value);
    [[nodiscard]] Result<void> patch(std::size_t binding, materials::Properties properties);
    // Links slots to one named material; supplied properties create it only when absent.
    [[nodiscard]] Result<void> assign(
        std::span<const std::size_t> bindings, const std::string& material,
        materials::Properties definition = {}
    );
    [[nodiscard]] Result<std::string> make_unique(std::span<const std::size_t> bindings);
    // Applies one shared preset field to every matching binding, using the same validation as object edits.
    [[nodiscard]] Result<void> patch_preset(
        const std::string& preset, std::string key, materials::Value value
    );
    [[nodiscard]] Result<void> patch(
        std::span<const std::size_t> bindings, materials::Properties properties
    );
    [[nodiscard]] Result<void> delete_override(const std::string& id);
    // Removes saved and unsaved exact-slot edits/assignments, retaining shared definitions and defaults.
    [[nodiscard]] Result<void> clear_slots(std::span<const std::size_t> bindings);
    // Removes one unsaved field, inheriting its last loaded/saved source again.
    [[nodiscard]] Result<void> undo(std::size_t binding, const std::string& key);
    [[nodiscard]] Result<void> undo(
        std::span<const std::size_t> bindings, const std::string& key
    );
    // Removes one unsaved shared-preset field without affecting other edited fields.
    [[nodiscard]] Result<void> undo_preset(const std::string& preset, const std::string& key);
    // Restores loaded values and discards unsaved edits without changing any JSON files.
    void reset();
    // Atomically writes only the local authored overlay, retaining the previous file on failure.
    [[nodiscard]] Result<void> save();

private:
    // Resolves an edited catalog transactionally before publishing any typed state.
    [[nodiscard]] Result<void> preview(
        materials::Catalog edits, const std::set<std::string>& deleted
    );
    // Resolves the complete editable file at its shared preview and save priority.
    [[nodiscard]] Result<std::shared_ptr<const scene::MaterialBindings>> resolve_author(
        const materials::Catalog& author, const std::set<std::string>& deleted
    ) const;
    // Compiles typed responses and publishes an immutable inspector snapshot after successful resolution.
    void publish(std::shared_ptr<const scene::MaterialBindings> bindings);
    std::shared_ptr<const scene::MaterialBindings> m_baseline;  // Last loaded/saved preview.
    std::shared_ptr<const MaterialEditor> m_editor;             // Current immutable UI snapshot.
    materials::Catalog m_author;                                // Existing local file plus edits.
    materials::Catalog m_edits;                                 // Sparse unsaved session rules.
    std::map<std::string, materials::Properties> m_created_materials;  // Initial values for field Revert.
    materials::Catalog m_inherited;  // Layers below the current save file.
    std::set<std::string> m_deleted;
    std::string
        m_transport_baseline;  // Loaded transport identity, unchanged by saving appearance edits.
    std::filesystem::path m_path;               // Authoring target from source precedence.
    std::vector<MaterialResponse> m_responses;  // Parallel to resolved scene bindings.
};

// Compiles one resolved property map, preserving the existing defaults for missing fields.
[[nodiscard]] MaterialResponse material_response(
    const scene::MaterialSource& source, const materials::Properties& properties
);
}  // namespace openrac::enhanced
