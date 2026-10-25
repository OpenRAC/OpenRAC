// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <map>
#include <span>
#include <vector>

#include "renderer/renderer.h"

namespace openrac::enhanced {
class WaterMesh {
public:
    WaterMesh(const std::filesystem::path& bake, const std::array<char, 32>& fingerprint);
    bool
    build(const renderer::EffectStrip&, std::span<const renderer::EffectVertex>, std::vector<renderer::EffectVertex>&);
    void clear_cache();

private:
    float rest_height(const renderer::EffectVertex&, unsigned surface) const;
    std::map<std::array<int, 4>, std::vector<std::array<float, 2>>> m_templates;
    bool m_missing_template = false;

    struct CachedMesh {
        std::vector<renderer::EffectVertex> source, triangles;
    };

    std::map<std::uint64_t, CachedMesh> m_mesh_cache;
    std::size_t m_cached_vertices = 0;

    struct RestTriangle {
        unsigned surface;
        std::array<renderer::Vec3, 3> p;
    };

    std::vector<RestTriangle> m_rest;
    std::map<std::pair<int, int>, std::vector<unsigned>> m_rest_tiles;
};
}  // namespace openrac::enhanced
