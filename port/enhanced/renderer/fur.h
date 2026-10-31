// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Verified fur coverage, directional surface detail and lighting.
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
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
        const std::optional<scene::Instance>& hero
    );  // Selects the current body's palette.
    void bind(
        const renderer::Shader& shader
    ) const;         // Supplies maps to production colour variants.
    void release();  // Called with the owning render context current.

private:
    std::optional<FurSurface> m_surface;
    unsigned m_map = 0;
    unsigned m_detail = 0;
    unsigned m_vertices = 0;
    int m_palette = -1;
};
}  // namespace openrac::enhanced
