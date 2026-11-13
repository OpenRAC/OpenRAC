// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "renderer/math.h"

namespace openrac::renderer {

// A posed retail shadow proxy, in world units. Nonpositive segment counts
// describe flat capsule ends; negative counts extend the end by -count/256.
struct ShadowCapsule {
    Vec3 a{}, b{};
    float radius_a = 0, radius_b = 0;
    int segments_a = 0, segments_b = 0;
};

// One more caster with its own direction and slab, for a game in which any moby may cast one
// (GameProfile::every_moby_casts_shadow); `moby` identifies it from one tick to the next.
struct ShadowSet {
    std::uint32_t moby = 0;
    std::vector<ShadowCapsule> capsules;
    Vec3 extrusion{};
    std::optional<std::array<float, 2>> heights;
};

// Closed, consistently wound prisms of the proxies' silhouettes perpendicular
// to extrusion. Caps make depth-fail stencil counting work inside a volume too.
std::vector<Vec3> shadow_volumes(
    std::span<const ShadowCapsule> capsules,
    Vec3 extrusion,
    std::optional<std::array<float, 2>> heights = std::nullopt
);

}  // namespace openrac::renderer
