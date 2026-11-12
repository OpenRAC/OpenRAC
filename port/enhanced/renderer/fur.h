// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Verified fur coverage, directional surface detail and lighting.
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "renderer/shader.h"
#include "scene/level.h"

namespace openrac::enhanced {
enum class FurDebug {
    Off,
    Mask,
    Direction,
    DetailNormals
};
// Reads or selects a session-only diagnostic; invalid values select Off.
[[nodiscard]] FurDebug fur_debug_view();
void set_fur_debug_view(FurDebug view);

// Session-only short-fur detail, directional highlights and scattering.
struct FurMaterialSettings {
    bool enabled = true;
    float strength = .21f;
    float contrast = .036f;
    float fade_distance = 103;
    bool lighting_enabled = true;
    float highlight = .52f;
    float softness = .53f;
    float directionality = .88f;
    float scattering = .33f;
    float environment_strength = 1.85f;
    bool shells_enabled = false;  // Optional geometry prototype, separate from surface shading.
    int shell_layers = 10;        // Bounded extra copies of the verified body surface.
    float shell_length = .02f;    // Fraction of the verified bind-pose body's largest extent.
    float shell_density = 128;    // Stable strand cells across the UV atlas.
    float shell_distance = 24;    // Geometry fades out before the surface material does.
};

[[nodiscard]] FurMaterialSettings fur_material_settings();
void set_fur_material_settings(FurMaterialSettings settings);
// Deterministic UV-space slopes and strand variation, generated along the authored groom.
[[nodiscard]] renderer::Rgba8Image make_fur_detail_map(const renderer::Rgba8Image& groom);

// Authored coordinates use the original atlas's top-left pixel origin, including flow +V down.
struct FurRegion {
    std::vector<std::array<float, 2>> polygon;
    std::array<float, 2> direction{};
    bool exclude = false;
};

struct FurPatch {
    // Inclusive triangle-number ranges in the verified body material, not copied geometry.
    std::vector<std::array<unsigned, 2>> triangles;
    std::array<float, 2> direction{};
};

// Asset identity and authored selections, verified against the body's topology before use.
struct FurProfile {
    std::string game, material;
    int class_id = -1;
    int width = 0, height = 0;
    unsigned index_count = 0, vertex_count = 0;
    std::uint64_t uv_signature = 0;
    std::vector<FurPatch> patches;
    std::vector<FurRegion> regions;
};

// Built-in original-atlas profile, authored from the exported rac1 body mesh.
[[nodiscard]] const FurProfile& rac1_fur_profile();
// Going Commando's, generated from its exported body (profiles/rac2/fur.json).
[[nodiscard]] const FurProfile& rac2_fur_profile();
// The built-in profile of a level's game ("rac1", "rac2"); an empty one for any other game.
[[nodiscard]] const FurProfile& fur_profile(std::string_view game);
// Hashes UV float words in triangle order; zero denotes invalid indices or non-finite UVs.
[[nodiscard]] std::uint64_t fur_uv_signature(
    const scene::LevelData& level, const scene::Primitive& primitive
);

// Verified material, its vertex range and the eligible fur vertices within that range.
struct FurSurface {
    unsigned model = 0, first = 0, count = 0;
    unsigned primitive = 0;
    std::vector<unsigned>
        vertices;  // Eligible vertices within first/count; no mixed island sharing.
};

// Unknown layouts, replacement assets and stand-ins opt out rather than guessing from colour.
[[nodiscard]] std::optional<FurSurface> identify_fur_surface(
    const scene::LevelData& level, const FurProfile& profile
);
// R: coverage; GB: signed UV direction, premultiplied by coverage and biased to UNORM; A: 255.
[[nodiscard]] renderer::Rgba8Image make_fur_map(
    const scene::LevelData& level, const scene::Primitive& primitive, const FurProfile& profile
);

// Render-thread ownership of a map and eligibility buffer, including complete fallback resources.
class HeroFur {
public:
    ~HeroFur();
    HeroFur() = default;
    HeroFur(const HeroFur&) = delete;
    HeroFur& operator=(const HeroFur&) = delete;
    void load(
        const scene::LevelData& level, const FurProfile& profile
    );  // Unknown layouts opt out.
    void prepare(
        const std::optional<scene::Instance>& hero, const renderer::Vec3& eye = {}
    );  // Selects the current body's palette.
    void bind(
        const renderer::Shader& shader
    ) const;  // Supplies maps to production colour variants.
    // Binds cached per-material controls; coverage resources and eligibility remain unchanged.
    void bind(const renderer::Shader& shader, const FurMaterialSettings& settings) const;
    // Updates optical uniforms without rebinding coverage maps or eligibility buffers.
    void bind_material(const renderer::Shader& shader, const FurMaterialSettings& settings) const;
    // True for the verified body's model, the only one whose vertices can carry fur.
    [[nodiscard]] bool hero_model(std::uint32_t model) const {
        return m_surface && m_palette >= 0 && model == m_surface->model;
    }
    // Any other model draws with every fur mode off, so its pipelines carry no fur code.
    void bind_model(
        const renderer::Shader& shader, std::uint32_t model, const FurMaterialSettings& settings
    ) const;
    // Only the current verified hero model can submit extra surface layers.
    [[nodiscard]] int shell_layers(
        std::uint32_t model, std::uint32_t first, std::uint32_t count
    ) const;
    // Zero restores the original surface; positive indices select successive coat slices.
    void bind_shell(const renderer::Shader& shader, int layer) const;
    void release();  // Called with the owning render context current.

private:
    std::optional<FurSurface> m_surface;
    unsigned m_map = 0;
    unsigned m_detail = 0;
    unsigned m_vertices = 0;
    int m_palette = -1;
    std::uint32_t m_first_index = 0, m_index_count = 0;  // Verified material's index range.
    float m_eye_distance = 0;  // Shared fade for colour and current/previous velocity poses.
    float m_body_extent = 0;   // Coat length converted to the model's own vertex units.
};
}  // namespace openrac::enhanced
