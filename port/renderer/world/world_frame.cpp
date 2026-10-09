// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-engine/src/game_camera.rs:
// ISC License, Copyright (c) 2026 ReRAC contributors.

#include "renderer/world/world_frame.h"

namespace openrac::renderer::world {

namespace {

// The projection's Z lane and the post-divide Z offset (UpdateViewContext,
// ReRAC's reading of the NTSC-U executable).
constexpr double kZScale = -8388080.0;
constexpr double kZOffset = 8388112.0;
constexpr double kDepthScale = 1.0 / 16777216.0;  // 2^-24

}  // namespace

std::array<Vec3, 3> WorldCamera::vu_rows() const {
    return {
        Vec3{-left[0], -left[1], -left[2]},
        Vec3{-up[0], -up[1], -up[2]},
        forward,
    };
}

Vec3 WorldCamera::to_camera(const Vec3& p) const {
    const Vec3 d{p[0] - eye[0], p[1] - eye[1], p[2] - eye[2]};
    const auto rows = vu_rows();
    return {dot(rows[0], d), dot(rows[1], d), dot(rows[2], d)};
}

WorldCamera WorldCamera::from(const Camera& camera) {
    const Mat4& v = camera.view;
    WorldCamera c;
    c.eye = camera.position;
    // The view's rows are right, up and -forward (renderer/math.h look_along).
    c.left = {-v[0], -v[4], -v[8]};
    c.up = {v[1], v[5], v[9]};
    c.forward = {-v[2], -v[6], -v[10]};
    const Mat4& p = camera.projection;
    if (p[0] != 0.0f && p[5] != 0.0f) {
        c.tan_x = 1.0f / p[0];
        c.tan_y = 1.0f / p[5];
    }
    return c;
}

Mat4 game_projection(float tan_x, float tan_y) {
    // The GS Z of a point at view depth d (game units) is a + b / d with a and
    // b from the projection's entries (ReRAC's z_affine, in double because
    // Z_OFFSET + n * A cancels to about -688). GL's window depth is
    // (ndc_z + 1) / 2, so ndc_z = 2 * Z / 2^24 - 1 and, with clip.w = d,
    // clip.z = (2 a / 2^24 - 1) * d + 2 b / 2^24.
    const double n = kNear;
    const double f = kFar;
    const double a_m = (f + n) / (n * (f - n)) * kZScale;
    const double c_m = (n * -2.0 * f) / (n * (f - n)) * kZScale;
    const double a = n * a_m + kZOffset;
    const double b = n * c_m / kUnits;
    const auto z_coefficient = static_cast<float>(-(2.0 * a * kDepthScale - 1.0));
    const auto z_constant = static_cast<float>(2.0 * b * kDepthScale);
    return {
        1.0f / tan_x,
        0,
        0,
        0,
        0,
        1.0f / tan_y,
        0,
        0,
        0,
        0,
        z_coefficient,
        -1.0f,
        0,
        0,
        z_constant,
        0,
    };
}

Camera game_camera(const Vec3& eye, const Vec3& forward, const Vec3& up, float tan_x, float tan_y) {
    Camera c;
    c.view = look_along(eye, forward, up);
    c.projection = game_projection(tan_x, tan_y);
    c.position = eye;
    return c;
}

}  // namespace openrac::renderer::world
