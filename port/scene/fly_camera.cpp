// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// FlyCamera movement and view construction shared by the viewer and in-game Freecam.

#include "scene/fly_camera.h"

#include <algorithm>
#include <cmath>

namespace openrac::scene {

namespace {

constexpr Vec3 kUp{0.0f, 0.0f, 1.0f};
constexpr float kLookSpeed = 0.003f;  // radians a pixel
constexpr float kMaxPitch = 1.55f;    // just short of straight up or down
constexpr float kFastFactor = 4.0f;
constexpr float kSlowFactor = 0.25f;  // Precision movement while Ctrl is held.

}  // namespace

// Matches the game's Z-up yaw and pitch convention.
Vec3 FlyCamera::forward() const {
    return {std::cos(pitch) * std::cos(yaw), std::cos(pitch) * std::sin(yaw), std::sin(pitch)};
}

// Builds the same rigid view for viewer and in-game inspection.
Mat4 FlyCamera::view() const {
    return renderer::look_along(position, forward(), kUp);
}

// Uses the renderer's reversed-depth lens.
Mat4 FlyCamera::projection(float aspect) const {
    return renderer::perspective_reversed(fov_y, aspect, near_z, far_z);
}

// Advances independently of simulation ticks using the caller's elapsed display time.
void FlyCamera::update(const FlyControls& c, float seconds) {
    yaw -= c.look_x * kLookSpeed;
    pitch = std::clamp(pitch - c.look_y * kLookSpeed, -kMaxPitch, kMaxPitch);
    const Vec3 f = forward();
    const Vec3 r = renderer::normalize(renderer::cross(f, kUp));
    Vec3 move{};
    const float ahead = float(c.forward) - float(c.back);
    const float sideways = float(c.right) - float(c.left);
    const float vertical = float(c.up) - float(c.down);
    for (unsigned axis = 0; axis < 3; ++axis) {
        move[axis] = f[axis] * ahead + r[axis] * sideways + kUp[axis] * vertical;
    }
    // The in-game freecam keeps combined movement at the selected speed.
    if (c.normalise_movement && renderer::dot(move, move) > 0) {
        move = renderer::normalize(move);
    }
    // Precision movement wins when both speed modifiers are held.
    float factor = c.fast ? kFastFactor : 1.0f;
    if (c.slow) {
        factor = kSlowFactor;
    }
    const float step = speed * seconds * factor;
    for (int i = 0; i < 3; ++i) {
        position[i] += move[i] * step;
    }
}

// Fits a complete exported level when the standalone viewer starts.
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

}  // namespace openrac::scene
