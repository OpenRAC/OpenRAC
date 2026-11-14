// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// CPU material identities and resolved surface records, shared by enhanced rendering and probe transport.
#pragma once
#include <map>
#include <memory>
#include <vector>

#include "materials/catalog.h"
#include "scene/level.h"

namespace openrac::scene {
// Stable identity of one exported model/slot; instances of the same type share its binding.
struct MaterialSource {
    std::uint32_t model = 0;       // Exported model index.
    int material = -1;             // Original material index.
    std::string model_name;        // Stable model label for authoring and bake fingerprints.
    materials::Identity identity;  // Layer/type/slot/texture matching fields.
    bool transport = false;  // At least one eligible static transport instance uses this model.
};

// Immutable level-load inputs; live edits reuse these hashes and originals rather than rereading assets.
struct MaterialAssets {
    std::vector<Material> originals;      // Base materials before enhanced overrides.
    std::vector<MaterialSource> sources;  // Unique model/slot identities.
    std::map<int, std::string>
        image_hashes;  // Decoded content for explicitly referenced bake emission images.
    std::map<std::string, int> images;  // Logical exported paths to already-loaded emission images.
};

// One immutable render record shared by equivalent mesh-slot bindings.
struct MaterialRecord {
    Material surface;
    std::uint64_t variant = 0;
};

// A mesh slot references a shared record and keeps its own authoring provenance.
struct MaterialBinding {
    std::shared_ptr<const MaterialRecord> record;
    materials::Resolved values;  // Effective shading properties and inspector provenance.
    const Material& surface() const { return record->surface; }
    std::uint64_t variant() const { return record->variant; }
};

// Cached scene bindings; no JSON, texture hashing or selector matching occurs during drawing.
struct MaterialBindings {
    materials::Catalog source_catalog;  // Source rules before material-definition expansion.
    materials::Catalog catalog;  // Resolved source layers used by the material inspector.
    std::shared_ptr<const MaterialAssets> assets;  // Immutable original identities and surfaces.
    std::vector<MaterialBinding> bindings;         // Parallel to assets->sources.
    std::vector<std::shared_ptr<const MaterialRecord>> records;  // Unique render records.
    std::map<std::pair<std::uint32_t, int>, std::size_t> lookup;  // Model/slot to cached binding.
    std::string transport_stamp;  // Empty if effective static transport equals original inputs.
    // Returns the resolved binding, or null for geometry not present in the loaded catalog.
    [[nodiscard]] const MaterialBinding* find(std::uint32_t model, int material) const;
    [[nodiscard]] bool original(std::uint32_t model, int material) const;
};

// Imports independently authored legacy emission fields, excluding unchanged generated defaults.
[[nodiscard]] Result<materials::Catalog> legacy_material_edits(const LevelData& level);
// Loads optional catalog layers in existing override order; malformed sources return explicit errors.
[[nodiscard]] Result<materials::Catalog> load_material_catalog(
    const LevelData& level, const std::filesystem::path& excluded = {}
);
// Returns the highest-priority authoring target, using the same path selection as the loader.
[[nodiscard]] std::filesystem::path material_author_path(const LevelData& level);
// Loads a referenced level-relative emission texture through the existing mod/texture-pack image path.
[[nodiscard]] Result<int> load_material_image(LevelData& level, const std::string& path);
// Builds immutable texture identities and slot/type information once per level.
[[nodiscard]] Result<std::shared_ptr<const MaterialAssets>> identify_materials(
    const LevelData& level, const materials::Catalog& catalog
);
// Resolves a complete edit atomically, sharing unchanged source assets with earlier versions.
[[nodiscard]] Result<std::shared_ptr<const MaterialBindings>> resolve_materials(
    std::shared_ptr<const MaterialAssets> assets, const materials::Catalog& catalog
);
// Prepares optional enhanced overrides; failures leave original geometry/materials usable.
void prepare_material_overrides(LevelData& level);
// Returns the same resolved material to runtime and transport, falling back to original fixture data.
[[nodiscard]] const Material& resolved_material(
    const LevelData& level, std::uint32_t model, int material
);
}  // namespace openrac::scene
