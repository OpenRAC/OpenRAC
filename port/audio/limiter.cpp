// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "audio/limiter.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "audio/mixer.h"

namespace openrac::audio {

void Limiter::process(std::span<float> interleaved) {
    static const float kRecover =
        1.0f - std::exp(-1.0f / (kReleaseSeconds * static_cast<float>(kOutputRate)));

    // Frame by frame: the gain the frame needs, at once, else back towards 1.
    for (std::size_t i = 0; i + 1 < interleaved.size(); i += 2) {
        const float peak = std::max(std::abs(interleaved[i]), std::abs(interleaved[i + 1]));
        m_gain += (1.0f - m_gain) * kRecover;

        // A frame over the ceiling at the gain now lowers it to what keeps it there.
        if (peak * m_gain > kCeiling) {
            m_gain = kCeiling / peak;
        }

        // Counted while it is turned down by more than a hundredth.
        if (m_gain < 0.99f) {
            ++m_taken.limited;
            m_taken.lowest = std::min(m_taken.lowest, m_gain);
        }

        interleaved[i] = std::clamp(interleaved[i] * m_gain, -1.0f, 1.0f);
        interleaved[i + 1] = std::clamp(interleaved[i + 1] * m_gain, -1.0f, 1.0f);
    }
}

Limiter::Taken Limiter::take() {
    return std::exchange(m_taken, {});
}

void to_int16(std::span<const float> in, std::span<std::int16_t> out) {
    const std::size_t n = std::min(in.size(), out.size());

    // Each sample scaled to 16 bits, rounded, and clamped against anything not limited.
    for (std::size_t i = 0; i < n; ++i) {
        const float v = std::round(in[i] * 32767.0f);
        out[i] = static_cast<std::int16_t>(std::clamp(v, -32768.0f, 32767.0f));
    }
}

}  // namespace openrac::audio
