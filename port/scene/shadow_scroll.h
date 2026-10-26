// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "renderer/renderer.h"

namespace openrac::scene {

// A logical update rectangle and its contiguous destination in the toroidal image.
struct ShadowScrollRegion {
    std::array<int, 4> logical{};
    std::array<int, 4> physical{};
};

struct ShadowScrollUpdate {
    renderer::Mat4 matrix = renderer::identity();
    std::array<std::int64_t, 2> origin{};
    std::array<int, 2> offset{};
    std::vector<ShadowScrollRegion> regions;
    bool full = false;
};

// Positive modulo also handles negative world-space texel coordinates.
int shadow_wrap(std::int64_t coordinate, int resolution);

// Newly exposed strips, split at physical wrap seams, with no duplicate corner updates.
std::vector<ShadowScrollRegion> shadow_scroll_regions(
    std::array<std::int64_t, 2> previous,
    std::array<std::int64_t, 2> next,
    int resolution,
    bool full
);

// Maps a dirty logical region into its physical image coordinates without wrapping vertices.
renderer::Mat4 shadow_scroll_matrix(
    const ShadowScrollUpdate& update, const ShadowScrollRegion& region, int resolution
);

// Narrows caster culling to a scissored rectangle while retaining the full light-depth interval.
renderer::Mat4 shadow_region_clip(
    const renderer::Mat4& matrix, const std::array<int, 4>& rect, int resolution
);

class ShadowScrollGrid {
public:
    // Extent and depth stay fixed until coverage, direction or scene changes require a refresh.
    ShadowScrollUpdate update(
        const renderer::Mat4& requested,
        renderer::Vec3 direction,
        renderer::Vec3 low,
        renderer::Vec3 high,
        int resolution,
        std::uint64_t generation,
        bool force
    );

    void invalidate() { m_valid = false; }

private:
    bool m_valid = false;
    renderer::Vec3 m_direction{};
    renderer::Mat4 m_basis = renderer::identity();
    float m_radius = 0, m_zlow = 0, m_zhigh = 0;
    std::uint64_t m_generation = 0;
    std::array<std::int64_t, 2> m_origin{};
    int m_resolution = 0;
};
}  // namespace openrac::scene
