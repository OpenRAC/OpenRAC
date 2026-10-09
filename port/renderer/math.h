// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The little matrix arithmetic the renderer and the viewer need: column-major
// 4 x 4 matrices (GL's order, and glTF's), in the game's axes (Z up).

#pragma once

#include <array>
#include <cmath>

namespace openrac::renderer {

using Vec3 = std::array<float, 3>;
using Mat4 = std::array<float, 16>;  // column-major: element (row r, column c) at c * 4 + r

constexpr Mat4 identity() {
    return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
}

constexpr Mat4 multiply(const Mat4& a, const Mat4& b) {
    Mat4 out{};
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) {
                sum += a[k * 4 + r] * b[c * 4 + k];
            }
            out[c * 4 + r] = sum;
        }
    }
    return out;
}

constexpr Vec3 transform_point(const Mat4& m, const Vec3& p) {
    return {
        m[0] * p[0] + m[4] * p[1] + m[8] * p[2] + m[12],
        m[1] * p[0] + m[5] * p[1] + m[9] * p[2] + m[13],
        m[2] * p[0] + m[6] * p[1] + m[10] * p[2] + m[14]
    };
}

constexpr Mat4 scale(float s) {
    return {s, 0, 0, 0, 0, s, 0, 0, 0, 0, s, 0, 0, 0, 0, 1};
}

constexpr Mat4 translation(const Vec3& t) {
    return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, t[0], t[1], t[2], 1};
}

inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}

inline float dot(const Vec3& a, const Vec3& b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

inline Vec3 normalize(const Vec3& v) {
    const float length = std::sqrt(dot(v, v));
    return length > 0.0f ? Vec3{v[0] / length, v[1] / length, v[2] / length} : v;
}

// A camera at `eye` looking along `forward`, with `up` roughly up: world to
// view space (GL's: X right, Y up, looking down -Z).
inline Mat4 look_along(const Vec3& eye, const Vec3& forward, const Vec3& up) {
    const Vec3 f = normalize(forward);
    const Vec3 s = normalize(cross(f, up));
    const Vec3 u = cross(s, f);
    return {
        s[0],
        u[0],
        -f[0],
        0,
        s[1],
        u[1],
        -f[1],
        0,
        s[2],
        u[2],
        -f[2],
        0,
        -dot(s, eye),
        -dot(u, eye),
        dot(f, eye),
        1
    };
}

// A perspective projection with reversed depth, the convention the game's
// depth test has (RENDERER.md section 5, OpenGOAL's renderer): the near plane
// lands at depth 1 and the far plane at 0, the depth buffer is cleared to 0
// and the test is GEQUAL. `fov_y` in radians.
inline Mat4 perspective_reversed(float fov_y, float aspect, float near_z, float far_z) {
    const float f = 1.0f / std::tan(fov_y / 2.0f);
    // GL's usual projection, with its Z row negated.
    return {
        f / aspect,
        0,
        0,
        0,
        0,
        f,
        0,
        0,
        0,
        0,
        -(far_z + near_z) / (near_z - far_z),
        -1,
        0,
        0,
        -(2.0f * far_z * near_z) / (near_z - far_z),
        0
    };
}

}  // namespace openrac::renderer
