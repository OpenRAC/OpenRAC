// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The late camera latch's fade (latch_fade.h).

#include "game/common/frontend/latch_fade.h"

#include <cmath>
#include <numbers>

namespace openrac::frontend {

float angle_between(float a, float b) {
    float d = std::fmod(a - b, 2.0f * std::numbers::pi_v<float>);
    if (d > std::numbers::pi_v<float>) {
        d -= 2.0f * std::numbers::pi_v<float>;
    } else if (d < -std::numbers::pi_v<float>) {
        d += 2.0f * std::numbers::pi_v<float>;
    }
    return d;
}

float LatchFade::frame(
    float target, bool on, bool new_tick, float tick_rate, double since, bool enabled
) {
    since = m_shown && since > 0.0 ? since : 0.0;
    if (new_tick) {
        m_rate = tick_rate;
    }
    m_carry *= static_cast<float>(std::exp(-since / kFadeSeconds));
    // With the latch off before and after, there is nothing to carry: the interpolated camera is
    // smooth by itself.
    const bool stepped = m_shown && (on || m_was_on) && (new_tick || on != m_was_on);
    if (stepped) {
        const float expected = m_heading + m_rate * static_cast<float>(since);
        const float step = angle_between(expected, target);
        m_carry = std::abs(step) < kCutRadians ? m_carry + step : 0.0f;
    }
    if (!enabled) {
        m_carry = 0.0f;
    }
    // Between ticks, measure how fast the target turns (not from a frame long ago).
    if (!new_tick && !stepped) {
        m_rate = since > 0.0 && since < 0.1
                     ? angle_between(target, m_heading) / static_cast<float>(since)
                     : 0.0f;
    }
    m_was_on = on;
    m_heading = target;
    m_shown = true;
    return target + m_carry;
}

}  // namespace openrac::frontend
