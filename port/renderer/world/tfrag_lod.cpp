// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-engine/src/tfrag_lod.rs and
// crates/rc-formats/src/tfrag.rs: ISC License, Copyright (c) 2026 ReRAC contributors.

#include "renderer/world/tfrag_lod.h"

#include <cmath>

namespace openrac::renderer::world {

namespace {

// The VU's CLIP judgement of one vertex against |w|: +x, -x, +y, -y, +z, -z.
std::uint32_t clip_flags(float x, float y, float z, float w) {
    const float aw = std::fabs(w);
    return static_cast<std::uint32_t>(x > aw) | static_cast<std::uint32_t>(x < -aw) << 1
           | static_cast<std::uint32_t>(y > aw) << 2 | static_cast<std::uint32_t>(y < -aw) << 3
           | static_cast<std::uint32_t>(z > aw) << 4 | static_cast<std::uint32_t>(z < -aw) << 5;
}

}  // namespace

int tfrag_mode_list(std::uint32_t mode) {
    if (mode == 6) {
        return 2;
    }
    if (mode == 8 || mode == 0xa) {
        return 1;
    }
    return 0;
}

std::array<float, 3> tfrag_lod_distances(float lod_base) {
    return {lod_base * 6.0f, lod_base * 4.0f, lod_base + lod_base};
}

std::array<std::int32_t, 3> tfrag_lod_thresholds(float lod_base) {
    const auto d = tfrag_lod_distances(lod_base);
    return {
        static_cast<std::int32_t>(d[0] * 1024.0f),
        static_cast<std::int32_t>(d[1] * 1024.0f),
        static_cast<std::int32_t>(d[2] * 1024.0f),
    };
}

std::uint32_t tfrag_lod_mode(
    float centre_depth, float radius, bool base_only, const std::array<std::int32_t, 3>& thresholds
) {
    const auto far = static_cast<std::int32_t>(centre_depth + radius);
    const auto near = static_cast<std::int32_t>(centre_depth - radius);
    const auto [d0, d1, d2] = thresholds;
    if (base_only || near >= d0) {
        return 6;
    }
    if (far >= d0) {
        return 8;
    }
    if (near >= d1) {
        return 0xa;
    }
    if (far >= d1) {
        return 0xe;
    }
    if (near >= d2 || far >= d2) {
        return 0x10;
    }
    return 0x14;
}

std::uint32_t tfrag_proc(
    const TfragCull& tfrag,
    const Vec3& camera_int,
    const std::array<Vec3, 3>& rows,
    const std::array<std::int32_t, 3>& thresholds,
    float tan_x,
    float tan_y,
    float far_cull
) {
    const float kx = std::sqrt(1.0f + tan_x * tan_x);
    const float ky = std::sqrt(1.0f + tan_y * tan_y);
    const auto& s = tfrag.sphere;
    const Vec3 d{s[0] - camera_int[0], s[1] - camera_int[1], s[2] - camera_int[2]};
    const Vec3 p{dot(rows[0], d), dot(rows[1], d), dot(rows[2], d)};
    const float r = s[3];
    const float far = r + p[2];
    const float near = -r + p[2];
    if (far_cull - near < 0.0f || kNear - far >= 0.0f) {
        return kTfragCulled;
    }
    const float ax = std::fabs(p[0]);
    const float ay = std::fabs(p[1]);
    if (tan_x * p[2] - (ax - r * kx) < 0.0f || tan_y * p[2] - (ay - r * ky) < 0.0f) {
        return kTfragCulled;
    }
    const bool inside = tan_x * p[2] - (ax + r * kx) >= 0.0f
                        && tan_y * p[2] - (ay + r * ky) >= 0.0f && far - kFar < 0.0f
                        && near - kNear >= 0.0f;
    if (!inside) {
        std::uint32_t all_out = 0x3f;
        std::uint32_t any_guard = 0;
        const float a = (kFar + kNear) / (kNear * (kFar - kNear));
        const float b = (kNear * -2.0f * kFar) / (kNear * (kFar - kNear));
        for (const auto& c : tfrag.corners) {
            const Vec3 dc{c[0] - camera_int[0], c[1] - camera_int[1], c[2] - camera_int[2]};
            const Vec3 v{dot(rows[0], dc), dot(rows[1], dc), dot(rows[2], dc)};
            const float cx = v[0] * (1.0f / (tan_x * kNear));
            const float cy = v[1] * (1.0f / (tan_y * kNear));
            const float cz = v[2] * a + b;
            const float cw = v[2] * (1.0f / kNear);
            all_out &= clip_flags(cx, cy, cz, cw);
            any_guard |= clip_flags(cx * 0.25f, cy * 0.25f, cz, cw);
        }
        if (all_out != 0) {
            return kTfragCulled;
        }
        if (any_guard != 0) {
            return kTfragClip;
        }
    }
    return tfrag_lod_mode(p[2], r, tfrag.base_only, thresholds);
}

TfragLodConstants tfrag_lod_constants(float lod_base, const WorldFog& fog) {
    const auto [d0, d1, d2] = tfrag_lod_distances(lod_base);
    const float cf20 = (fog.far_intensity - fog.near_intensity)
                       / ((fog.far_dist - fog.near_dist) * 0.0009765625f);
    const float f0 = d0 * cf20;
    const float f1 = d1 * cf20;
    const float f2 = d2 * cf20;
    const float a = 1.0f / (f0 - f1);
    const float b = 1.0f / (f1 - f2);
    TfragLodConstants k;
    k.k666 = {a * 0.5f, -a, 0.0f, f0};
    k.k667 = {b * 0.5f, -b, 0.0f, f1};
    k.k668 = {f1 * a * -0.5f, f0 * a, 0.0f, 0.0f};
    k.k669 = {f2 * b * -0.5f, f1 * b, 0.0f, 0.0f};
    k.w_slope = fog.terms().slope;
    return k;
}

void tfrag_modes(
    std::span<const TfragCull> tfrags,
    float lod_base,
    const WorldCamera& camera,
    std::span<const std::uint8_t> occlusion,
    float far_cull,
    std::span<std::uint32_t> out
) {
    const auto thresholds = tfrag_lod_thresholds(lod_base);
    const auto rows = camera.vu_rows();
    const Vec3 cam{camera.eye[0] * kUnits, camera.eye[1] * kUnits, camera.eye[2] * kUnits};
    for (std::size_t i = 0; i < tfrags.size() && i < out.size(); ++i) {
        if (!tfrags[i].occlusion.visible(occlusion)) {
            out[i] = kTfragCulled;
            continue;
        }
        out[i] = tfrag_proc(tfrags[i], cam, rows, thresholds, camera.tan_x, camera.tan_y, far_cull);
    }
}

}  // namespace openrac::renderer::world
