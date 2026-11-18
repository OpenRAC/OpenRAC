// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Implements rumble.h: the motors' envelopes and how the game's commands are mixed onto a gamepad.

#include "platform/rumble.h"

#include <algorithm>
#include <cmath>

namespace openrac::platform {

namespace {

// One envelope step towards `target`: the time constant `rise` going up, `fall` coming down.
float follow(float level, float target, float seconds, float rise, float fall) {
    const float tau = target > level ? rise : fall;
    const float alpha = 1.0f - std::exp(-seconds / tau);
    level += (target - level) * alpha;
    // The tail of a coast-down is let go, rather than buzzing faintly for ever.
    if (target <= 0.0f && level < RumbleShaper::kRest) {
        level = 0.0f;
    }
    return level;
}

float clamp01(float v) {
    return std::clamp(v, 0.0f, 1.0f);
}

}  // namespace

RumbleOutput RumbleShaper::step(
    const MotorCommand& command, float seconds, const RumbleTuning& tuning
) {
    // A long frame (a load, the window dragged) is one step of a tenth of a second at most.
    seconds = std::clamp(seconds, 0.0f, 0.1f);
    const float scale = static_cast<float>(std::clamp(tuning.strength, 0, 200)) / 100.0f;
    const float large = static_cast<float>(command.large) / 255.0f;
    RumbleOutput out;

    if (tuning.style == RumbleStyle::Classic) {
        reset();
        out.low = clamp01(large * scale);
        out.high = command.small ? clamp01(scale) : 0.0f;
        if (tuning.triggers && command.small) {
            out.left_trigger = out.right_trigger = clamp01(kTriggerLevel * scale);
        }
        return out;
    }

    const float low_target = large;
    const float high_target = command.small ? kSmallLevel : 0.0f;
    // A motor that starts from rest is kicked.
    if (low_target > 0.0f && m_low < kRest) {
        m_low_kick = kKickTime;
    }
    if (high_target > 0.0f && m_high < kRest) {
        m_high_kick = kKickTime;
    }
    m_low = follow(m_low, low_target, seconds, kLowRise, kLowFall);
    m_high = follow(m_high, high_target, seconds, kHighRise, kHighFall);

    // While kicked, a motor is driven at its target times the gain, or its envelope if that is
    // more (a command that rose further during the kick).
    float low = m_low;
    if (m_low_kick > 0.0f) {
        low = low_target > 0.0f ? std::max(m_low, low_target * kKickGain) : m_low;
        m_low_kick = std::max(0.0f, m_low_kick - seconds);
    }
    float high = m_high;
    if (m_high_kick > 0.0f) {
        high = high_target > 0.0f ? std::max(m_high, high_target * kKickGain) : m_high;
        m_high_kick = std::max(0.0f, m_high_kick - seconds);
    }

    out.low = clamp01(low * scale);
    out.high = clamp01((high + kTexture * m_low) * scale);
    if (tuning.triggers) {
        const float buzz = kTriggerLevel * (m_high / kSmallLevel + 0.5f * m_low);
        out.left_trigger = out.right_trigger = clamp01(buzz * scale);
    }
    return out;
}

void RumbleShaper::reset() {
    m_low = m_high = 0.0f;
    m_low_kick = m_high_kick = 0.0f;
}

std::uint16_t rumble_word(float strength) {
    return static_cast<std::uint16_t>(std::lround(clamp01(strength) * 65535.0f));
}

bool rumble_changed(const RumbleOutput& a, const RumbleOutput& b) {
    constexpr float kStep = 0.01f;
    return std::fabs(a.low - b.low) > kStep || std::fabs(a.high - b.high) > kStep
           || std::fabs(a.left_trigger - b.left_trigger) > kStep
           || std::fabs(a.right_trigger - b.right_trigger) > kStep || a.silent() != b.silent();
}

}  // namespace openrac::platform
