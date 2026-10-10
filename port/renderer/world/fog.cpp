// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-engine/src/game_camera.rs
// and crates/rc-engine/src/tie_lod.rs: ISC License, Copyright (c) 2026 ReRAC contributors.

#include "renderer/world/fog.h"

#include <algorithm>

#include "renderer/world/vu_float.h"

namespace openrac::renderer::world {

WorldFog::Terms WorldFog::terms() const {
    const float range = far_dist - near_dist;
    const float q_numerator = ((far_intensity - near_intensity) * kNear) / range;
    const float slope = (1.0f / kNear) * q_numerator;
    const float offset = (near_intensity * far_dist - far_intensity * near_dist) / range;
    return {q_numerator, slope, offset};
}

float WorldFog::vertex_f(float depth_int) const {
    const Terms t = terms();
    return std::max(std::min(depth_int * t.slope + t.offset, near_intensity), far_intensity);
}

std::array<float, 4> fog_params(const WorldFog& fog) {
    const WorldFog::Terms t = fog.terms();
    return {t.slope, t.offset, fog.far_intensity, fog.near_intensity};
}

std::uint32_t tie_fog_value(float depth, const WorldFog& fog) {
    using namespace vu;
    const std::uint32_t dn = bits(fog.near_dist);
    const std::uint32_t df = bits(fog.far_dist);
    const std::uint32_t i_n = bits(fog.near_intensity);
    const std::uint32_t i_f = bits(fog.far_intensity);
    const std::uint32_t per_unit = 0x3a800000u;  // 1/1024
    const std::uint32_t range = sub(df, dn);
    const std::uint32_t cf20 = div(sub(i_f, i_n), mul(range, per_unit));
    const std::uint32_t cf24 = sub(i_n, mul(mul(dn, per_unit), cf20));
    const std::uint32_t f = max(min(add(cf24, mul(bits(depth), cf20)), i_n), i_f);
    return static_cast<std::uint32_t>(std::clamp(value(f), 0.0f, 255.0f));
}

}  // namespace openrac::renderer::world
