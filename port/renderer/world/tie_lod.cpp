// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-engine/src/tie_lod.rs:
// ISC License, Copyright (c) 2026 ReRAC contributors.

#include "renderer/world/tie_lod.h"

#include <cmath>

#include "renderer/world/vu_float.h"

namespace openrac::renderer::world {

namespace {

constexpr std::uint32_t k256 = 0x43800000u;  // 256.0 (lui at, 0x4380)

TieLodPick fixed(std::uint32_t lod) {
    // The static paths store qw4 = (0, 0, 256.0, 0).
    return {lod, 0.0f, 0.0f, 256.0f};
}

TieLodPick morph(std::uint32_t lod, std::uint32_t num, std::uint32_t den) {
    // vdiv Q, then qw4 = (Q, 0, 256 - 256 Q, 256 Q) on VU0.
    const std::uint32_t q = vu::div(num, den);
    const std::uint32_t w = vu::mul(k256, q);
    const std::uint32_t z = vu::sub(k256, w);
    return {lod, vu::value(q), vu::value(w), vu::value(z)};
}

}  // namespace

TieLodPick tie_lod(float depth, const std::array<float, 3>& distances) {
    const std::uint32_t near = vu::bits(distances[0]);
    const std::uint32_t mid = vu::bits(distances[1]);
    const std::uint32_t far = vu::bits(distances[2]);
    const std::uint32_t d = vu::bits(depth);
    const std::uint32_t dn = vu::sub(near, d);
    const std::uint32_t dm = vu::sub(mid, d);
    const std::uint32_t df = vu::sub(far, d);
    auto negative = [](std::uint32_t x) {
        return (x & vu::kSign) != 0;
    };
    // By sign bit, in the game's order: far < depth wins over depth <= mid.
    if (!negative(dn)) {
        return fixed(0);
    }
    if (negative(df)) {
        return fixed(2);
    }
    if (!negative(dm)) {
        return morph(0, vu::sub(0, dn), vu::sub(mid, near));
    }
    return morph(1, vu::sub(0, dm), vu::sub(far, mid));
}

std::optional<float> tie_cull(const Vec3& p, float radius, float draw_distance, float tan_x, float tan_y) {
    if (vu::bits(draw_distance) == 0) {
        return std::nullopt;
    }
    if ((draw_distance - radius) - (p[2] - radius) < 0.0f || kNear / kUnits - (p[2] + radius) >= 0.0f) {
        return std::nullopt;
    }
    const float kx = std::sqrt(1.0f + tan_x * tan_x);
    const float ky = std::sqrt(1.0f + tan_y * tan_y);
    if (tan_x * p[2] - (std::fabs(p[0]) - radius * kx) < 0.0f
        || tan_y * p[2] - (std::fabs(p[1]) - radius * ky) < 0.0f) {
        return std::nullopt;
    }
    return p[2] > 0.0f ? p[2] : 0.0f;
}

std::array<std::uint32_t, 4> tie_record(
    const Vec3& camera_centre,
    float radius,
    float draw_distance,
    const std::array<float, 3>& distances,
    const WorldFog& fog,
    float tan_x,
    float tan_y
) {
    const auto depth = tie_cull(camera_centre, radius, draw_distance, tan_x, tan_y);
    if (!depth) {
        return {kTieCulled, 0, 0, 0};
    }
    const TieLodPick pick = tie_lod(*depth, distances);
    return {
        pick.lod | (tie_fog_value(*depth, fog) << 8),
        vu::bits(pick.k),
        vu::bits(pick.w),
        vu::bits(pick.z),
    };
}

}  // namespace openrac::renderer::world
