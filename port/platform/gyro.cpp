// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "platform/gyro.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace openrac::platform {
namespace {

Vec3f sub(Vec3f a, Vec3f b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

Vec3f min3(Vec3f a, Vec3f b) {
    return {std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)};
}

Vec3f max3(Vec3f a, Vec3f b) {
    return {std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)};
}

float largest(Vec3f a) {
    return std::max({std::fabs(a.x), std::fabs(a.y), std::fabs(a.z)});
}

float length(Vec3f a) {
    return std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}

// How much of the motion at `speed` goes straight through, the rest smoothed (GyroWiki's soft
// tiered smoothing): none at half the threshold and under, all at the threshold and over.
float direct_share(float speed, float threshold) {
    if (threshold <= 0.0f) {
        return 1.0f;
    }
    const float low = threshold * 0.5f;
    return std::clamp((speed - low) / (threshold - low), 0.0f, 1.0f);
}

}  // namespace

GyroSpace gyro_space_from(const char* text) {
    if (text == nullptr) {
        return GyroSpace::Player;
    }
    if (std::strcmp(text, "yaw") == 0) {
        return GyroSpace::Yaw;
    }
    if (std::strcmp(text, "roll") == 0) {
        return GyroSpace::Roll;
    }
    if (std::strcmp(text, "local") == 0) {
        return GyroSpace::Local;
    }
    return GyroSpace::Player;
}

GyroButtonAction gyro_button_action_from(const char* text) {
    if (text != nullptr && std::strcmp(text, "pause") == 0) {
        return GyroButtonAction::Pause;
    }
    if (text != nullptr && std::strcmp(text, "toggle") == 0) {
        return GyroButtonAction::Toggle;
    }
    return GyroButtonAction::Enable;
}

void GyroAim::reset() {
    const GyroOptions options = m_options;
    *this = GyroAim{};
    m_options = options;
}

void GyroAim::button(bool held) {
    if (m_options.button < 0) {
        m_button_on = true;
        m_button_was = held;
        return;
    }
    const bool was_on = m_button_on;
    switch (m_options.button_action) {
        case GyroButtonAction::Enable:
            m_button_on = held;
            break;
        case GyroButtonAction::Pause:
            m_button_on = !held;
            break;
        case GyroButtonAction::Toggle:
            if (held && !m_button_was) {
                m_toggled = !m_toggled;
            }
            m_button_on = m_toggled;
            break;
    }
    m_button_was = held;
    // Turned on or off: the smoothing starts afresh, no motion from before carries on.
    if (was_on != m_button_on) {
        m_smooth_yaw = m_smooth_pitch = 0.0f;
    }
}

void GyroAim::accel(Vec3f a) {
    m_last_accel = a;
    if (!m_has_gravity) {
        m_gravity = a;
        m_has_gravity = true;
        return;
    }
    // A plain low pass: the way up changes slowly, a shake of the hand quickly.
    constexpr float kStep = 1.0f / 250.0f;  // about a sample of a pad's 250 a second
    const float k = 1.0f - std::exp(-kStep / kGravitySeconds);
    m_gravity.x += (a.x - m_gravity.x) * k;
    m_gravity.y += (a.y - m_gravity.y) * k;
    m_gravity.z += (a.z - m_gravity.z) * k;
}

void GyroAim::learn_offset(Vec3f rate, Vec3f accel, float dt) {
    if (m_count == 0) {
        m_sum = {};
        m_min = m_max = rate;
        m_accel_min = m_accel_max = accel;
        m_still = 0.0f;
    }
    m_sum.x += rate.x;
    m_sum.y += rate.y;
    m_sum.z += rate.z;
    m_min = min3(m_min, rate);
    m_max = max3(m_max, rate);
    m_accel_min = min3(m_accel_min, accel);
    m_accel_max = max3(m_accel_max, accel);
    ++m_count;
    m_still += dt;

    const bool moved =
        largest(sub(m_max, m_min)) > kStillSpread
        || (m_has_gravity && largest(sub(m_accel_max, m_accel_min)) > kStillAccelSpread);
    if (moved) {
        m_count = 0;
        return;
    }
    if (m_still < kStillSeconds) {
        return;
    }
    const float n = static_cast<float>(m_count);
    const Vec3f mean{m_sum.x / n, m_sum.y / n, m_sum.z / n};
    m_count = 0;
    if (largest(mean) > kMaxOffset) {
        return;
    }
    // The first time, the mean; then halfway there, so one odd window does not throw it.
    if (!m_has_offset) {
        m_offset = mean;
        m_has_offset = true;
    } else {
        m_offset.x += (mean.x - m_offset.x) * 0.5f;
        m_offset.y += (mean.y - m_offset.y) * 0.5f;
        m_offset.z += (mean.z - m_offset.z) * 0.5f;
    }
}

void GyroAim::gyro(Vec3f rate, std::uint64_t ns) {
    float dt = 0.0f;
    if (m_has_last && ns > m_last_ns) {
        dt = static_cast<float>(static_cast<double>(ns - m_last_ns) * 1e-9);
    }
    m_last_ns = ns;
    m_has_last = true;
    if (dt <= 0.0f || dt > kMaxStep) {
        return;
    }

    if (m_options.auto_calibrate) {
        learn_offset(rate, m_last_accel, dt);
    }
    const Vec3f v = sub(rate, m_offset);

    // The turn left (about the way up) and up.
    float yaw = v.y;
    switch (m_options.space) {
        case GyroSpace::Yaw:
            yaw = v.y;
            break;
        case GyroSpace::Roll:
            yaw = v.z;
            break;
        case GyroSpace::Local:
            yaw = v.y + v.z;
            break;
        case GyroSpace::Player: {
            const float g = length(m_gravity);
            if (!m_has_gravity || g < 1.0f) {
                yaw = v.y;  // no accelerometer: as Yaw
                break;
            }
            // The turn about the way up, in the controller's y-z plane, a little relaxed so that a
            // controller held tilted still turns fully (GyroWiki's player space).
            const float world = (v.y * m_gravity.y + v.z * m_gravity.z) / g;
            const float most = std::sqrt(v.y * v.y + v.z * v.z);
            yaw = std::copysign(std::min(std::fabs(world) * kRelax, most), world);
            break;
        }
    }
    float pitch = v.x;

    if (!aiming()) {
        m_smooth_yaw = m_smooth_pitch = 0.0f;
        return;
    }

    // The dead zone, by speed: what is over it is scaled down so the turn starts smoothly.
    float speed = std::sqrt(yaw * yaw + pitch * pitch);
    if (m_options.dead_zone > 0.0f) {
        if (speed <= m_options.dead_zone) {
            yaw = pitch = 0.0f;
        } else {
            const float k = (speed - m_options.dead_zone) / speed;
            yaw *= k;
            pitch *= k;
        }
        speed = std::sqrt(yaw * yaw + pitch * pitch);
    }

    // The smallest motion through a short average (hand shake), the rest straight through.
    const float direct = direct_share(speed, m_options.smoothing);
    if (direct < 1.0f) {
        const float k = 1.0f - std::exp(-dt / kSmoothingSeconds);
        m_smooth_yaw += (yaw * (1.0f - direct) - m_smooth_yaw) * k;
        m_smooth_pitch += (pitch * (1.0f - direct) - m_smooth_pitch) * k;
        yaw = yaw * direct + m_smooth_yaw;
        pitch = pitch * direct + m_smooth_pitch;
    } else {
        m_smooth_yaw = m_smooth_pitch = 0.0f;
    }

    const float sx = std::max(m_options.sensitivity_x, 0.0f) * (m_options.invert_x ? -1.0f : 1.0f);
    const float sy = std::max(m_options.sensitivity_y, 0.0f) * (m_options.invert_y ? -1.0f : 1.0f);
    m_yaw += yaw * sx * dt;
    m_pitch += pitch * sy * dt;
}

}  // namespace openrac::platform
