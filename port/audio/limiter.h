// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The one limiter of the sound output: the float mix (voices, reverb, streams, the mods' and the
// menu's sounds) can sum past full scale, as many loud sounds at once do; it is brought under full
// scale here instead of being clipped. A peak limiter: the gain drops at once to what keeps the
// frame under the ceiling and recovers over kRelease, the same gain on both sides so that the
// stereo picture holds; a frame that is still over after that (none, but for safety) is clamped.
// Below the ceiling it changes nothing.

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace openrac::audio {

class Limiter {
public:
    // Just under full scale, so that the conversion to 16 bits never wraps.
    static constexpr float kCeiling = 0.98f;
    // The time the gain takes to come back most of the way (to within 1/e).
    static constexpr float kReleaseSeconds = 0.08f;

    // Interleaved stereo at 48 kHz, in place.
    void process(std::span<float> interleaved);

    float gain() const { return m_gain; }

    // The frames turned down and the lowest gain since the last take (the stats, OPENRAC_AUDIO_STATS).
    struct Taken {
        std::size_t limited = 0;
        float lowest = 1.0f;
    };

    Taken take();

private:
    float m_gain = 1.0f;
    Taken m_taken;
};

// Interleaved float (1 = full scale) to signed 16-bit, rounded and clamped.
void to_int16(std::span<const float> in, std::span<std::int16_t> out);

}  // namespace openrac::audio
