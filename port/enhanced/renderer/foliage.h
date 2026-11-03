// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Reviewed thin-leaf materials, identified by original decoded texture content.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "common/result.h"
#include "scene/level.h"

namespace openrac::json {
class Value;
}

namespace openrac::renderer {
class Shader;
}

namespace openrac::enhanced {
// Optical controls, independent of coverage alpha and the original painted colours.
struct FoliageMaterial {
    float transmission = .35f;  // Diffuse energy assigned to the opposite hemisphere, 0-1.
    float thickness = .7f;      // Dimensionless absorption distance, 0-4.
    float normal_bend =
        .2f;                 // Rounded UV-card normal strength, 0-1; zero retains authored normals.
    float roughness = .65f;  // Perceptual dielectric GGX roughness, 0.35-1.
};

// One reviewed texture's session controls and the loaded profile values used by Reset.
struct FoliageMaterialControls {
    std::string name;          // Human-readable profile label.
    std::string texture_hash;  // Immutable texture identity for stale-edit rejection.
    FoliageMaterial material;  // Current session values.
    FoliageMaterial defaults;  // Values loaded from the JSON profile.
    int matched_images = 0;    // Eligible decoded images using this texture.
};

// Owned UI snapshot; edits apply after render completion, never while the worker reads materials.
struct FoliageControls {
    bool enabled = false;                            // Current master switch.
    int debug = 0;                                   // 0 shaded, 1 mask, 2 transmission, 3 normals.
    bool default_enabled = false;                    // Profile's master switch.
    int default_debug = 0;                           // Profile's diagnostic view.
    std::vector<FoliageMaterialControls> materials;  // Reviewed textures in profile order.
};

// Returns a portable hash of little-endian dimensions and RGBA8 texels; empty for invalid images.
[[nodiscard]] std::string foliage_texture_hash(const renderer::Rgba8Image& image);
// Rejects nonfinite or out-of-range parameters instead of passing invalid material data to the GPU.
[[nodiscard]] Result<FoliageMaterial> read_foliage_material(const json::Value& value);
// Packs four bounded controls into two half pairs, retaining the existing 96-byte GPU material record.
[[nodiscard]] std::array<std::uint32_t, 2> pack_foliage_material(const FoliageMaterial& material);

// Render-thread material selection; no GPU resources or mutable state are shared with the game thread.
class FoliageMaterials {
public:
    // Loads optional exported/override profiles; errors leave foliage disabled without disabling other lighting.
    [[nodiscard]] Result<void> load(const scene::LevelData& level);
    // Returns zero controls for unselected, opaque, metal, blended or disabled surfaces.
    [[nodiscard]] std::array<std::uint32_t, 2> parameters(
        const scene::Material& material, bool active
    ) const;
    // Sets both packed words for every material, including zero for unselected or disabled surfaces.
    void bind(const renderer::Shader& shader, const scene::Material& material, bool active) const;

    // Returns an owned edit snapshot; the render worker must be idle.
    [[nodiscard]] FoliageControls controls() const { return m_controls; }

    // Validates the whole edit before replacing cached GPU words; rejects stale texture identities.
    [[nodiscard]] Result<void> set_controls(const FoliageControls& controls);
    // Restores the loaded profile's controls, without editing the JSON file; the worker must be idle.
    void reset_controls();
    // Drops the previous level's selections and diagnostics.
    void clear();

    // Returns the number of reviewed texture images matched in this level.
    [[nodiscard]] std::size_t size() const { return m_materials.size(); }

private:
    // Rebuilds packed words after validated session edits, without hashing textures again.
    void update_parameters();

    std::map<int, std::array<std::uint32_t, 2>>
        m_materials;                            // Packed controls by loaded image index.
    std::map<int, std::string> m_image_hashes;  // Level-load identities, reused by live edits.
    FoliageControls m_controls;  // Render-owned; UI accesses only after worker completion.
};
}  // namespace openrac::enhanced
