// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Cached GPU material responses and sparse authoring edits for one enhanced scene.
#pragma once
#include <filesystem>
#include <memory>
#include <vector>

#include "enhanced/renderer/foliage.h"
#include "enhanced/renderer/fur.h"
#include "enhanced/renderer/reflection_material.h"
#include "enhanced/renderer/wetness.h"
#include "scene/material_overrides.h"

namespace openrac::enhanced {
// Typed responses compiled at load/edit, never interpreted from JSON during drawing.
struct MaterialResponse {
    std::array<float, 3> sky_tint{1, 1, 1};  // RGB multiplier for the separate sky pass.
    ReflectionMaterialSettings reflection;   // Original reflection family with supplied overrides.
    FurMaterialSettings fur;                 // Applied only to verified coverage by HeroFur.
    WetnessMaterialSettings wetness;         // Applied only to HeroWetness's verified vertices.
    int fallback_channels = 0;               // Bit 0 diffuse scale, bit 1 authored emission.
    bool fur_override = false;               // Explicit fur fields select cached authored response.
    bool wetness_override = false;     // Explicit wetness fields select cached authored response.
    bool reflection_override = false;  // Authored optical fields can use the retail fallback.
    bool family_override = false;      // Explicit catalog family, including standard opt-outs.
    std::array<std::uint32_t, 2> foliage{};  // Existing packed optical controls.
};

// Immutable inspector snapshot, shared between frontend and scene while the render worker is idle.
struct MaterialEditor {
    std::shared_ptr<const scene::MaterialBindings> bindings;  // Current resolved preview.
    std::filesystem::path path;                               // Exact atomic-save target.
    std::string error;                                        // Load error, if any.
    std::size_t foliage_bindings = 0;  // Explicit thin-leaf bindings in this level.
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
    // Applies one shared preset field to every matching binding, using the same validation as object edits.
    [[nodiscard]] Result<void> patch_preset(
        const std::string& preset, std::string key, materials::Value value
    );
    // Removes one unsaved field, inheriting its last loaded/saved source again.
    [[nodiscard]] Result<void> undo(std::size_t binding, const std::string& key);
    // Removes one unsaved shared-preset field without affecting other edited fields.
    [[nodiscard]] Result<void> undo_preset(const std::string& preset, const std::string& key);
    // Restores loaded values and discards unsaved edits without changing any JSON files.
    void reset();
    // Atomically writes only the local authored overlay, retaining the previous file on failure.
    [[nodiscard]] Result<void> save();

private:
    // Resolves an edited catalog transactionally before publishing any typed state.
    [[nodiscard]] Result<void> preview(materials::Catalog edits);
    // Resolves the complete editable file at its shared preview and save priority.
    [[nodiscard]] Result<std::shared_ptr<const scene::MaterialBindings>> resolve_author(
        const materials::Catalog& author
    ) const;
    // Compiles typed responses and publishes an immutable inspector snapshot after successful resolution.
    void publish(std::shared_ptr<const scene::MaterialBindings> bindings);
    std::shared_ptr<const scene::MaterialBindings> m_baseline;  // Last loaded/saved preview.
    std::shared_ptr<const MaterialEditor> m_editor;             // Current immutable UI snapshot.
    materials::Catalog m_author;                                // Existing local file plus edits.
    materials::Catalog m_edits;                                 // Sparse unsaved session rules.
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
