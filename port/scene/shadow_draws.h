// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "scene/level.h"

namespace openrac::scene {

// Geometry and material classification only; visibility, instances and cascade state stay live.
struct ShadowDrawRange {
    std::uint32_t first = 0;
    std::uint32_t count = 0;
    int material = -1;  // Only alpha-tested ranges require a material.
    bool solid = false;
};

// Joins adjacent opaque ranges without crossing discarded materials or changing alpha coverage.
std::vector<ShadowDrawRange> prepare_shadow_draws(
    std::span<const Primitive> primitives,
    std::span<const Material> materials,
    const std::vector<bool>& solid_primitives,
    bool allow_solid
);

// Small consecutive opaque triangle ranges, with conservative local-space bounds.
// Alpha ranges remain whole, with bounds too. Invalid bounds always survive culling.
struct ShadowGeometryRange {
    ShadowDrawRange draw;
    renderer::Vec3 low{}, high{};
    bool bounded = false;
};

std::vector<ShadowGeometryRange> prepare_shadow_geometry(
    std::span<const ShadowDrawRange> draws,
    std::span<const Vertex> vertices,
    std::span<const std::uint32_t> indices
);

}  // namespace openrac::scene
