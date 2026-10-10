// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "viewer/camera.h"

#include <algorithm>
#include <cmath>

namespace openrac::viewer {

namespace {

constexpr Vec3 kUp{0.0f, 0.0f, 1.0f};
constexpr float kLookSpeed = 0.003f;  // radians a pixel
constexpr float kMaxPitch = 1.55f;    // just short of straight up or down
constexpr float kFastFactor = 4.0f;

}  // namespace

Vec3 FlyCamera::forward() const {
    return {std::cos(pitch) * std::cos(yaw), std::cos(pitch) * std::sin(yaw), std::sin(pitch)};
}

Mat4 FlyCamera::view() const {
    return renderer::look_along(position, forward(), kUp);
}

Mat4 FlyCamera::projection(float aspect) const {
    return renderer::perspective_reversed(fov_y, aspect, near_z, far_z);
}

void FlyCamera::update(const FlyControls& c, float seconds) {
    yaw -= c.look_x * kLookSpeed;
    pitch = std::clamp(pitch - c.look_y * kLookSpeed, -kMaxPitch, kMaxPitch);
    const Vec3 f = forward();
    const Vec3 r = renderer::normalize(renderer::cross(f, kUp));
    Vec3 move{};
    auto add = [&](const Vec3& d, float sign) {
        for (int i = 0; i < 3; ++i) {
            move[i] += d[i] * sign;
        }
    };
    if (c.forward) {
        add(f, 1.0f);
    }
    if (c.back) {
        add(f, -1.0f);
    }
    if (c.right) {
        add(r, 1.0f);
    }
    if (c.left) {
        add(r, -1.0f);
    }
    if (c.up) {
        add(kUp, 1.0f);
    }
    if (c.down) {
        add(kUp, -1.0f);
    }
    const float step = speed * seconds * (c.fast ? kFastFactor : 1.0f);
    for (int i = 0; i < 3; ++i) {
        position[i] += move[i] * step;
    }
}

void FlyCamera::frame(const Vec3& low, const Vec3& high) {
    const Vec3 centre{(low[0] + high[0]) / 2, (low[1] + high[1]) / 2, (low[2] + high[2]) / 2};
    const Vec3 size{high[0] - low[0], high[1] - low[1], high[2] - low[2]};
    const float radius = std::max(5.0f, std::sqrt(renderer::dot(size, size)) / 2.0f);
    yaw = 0.785398f;     // looking along +X +Y
    pitch = -0.523599f;  // 30 degrees down
    const Vec3 f = forward();
    const float distance = radius / std::tan(fov_y / 2.0f);
    for (int i = 0; i < 3; ++i) {
        position[i] = centre[i] - f[i] * distance;
    }
    far_z = std::max(far_z, distance + radius * 2.0f);
    speed = std::max(10.0f, radius / 8.0f);
}

}  // namespace openrac::viewer
