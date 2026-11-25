// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "audio/pitch.h"

#include <cmath>

namespace openrac::audio {

double note_speed(int center_note, int center_fine, int note, int fine) {
    const double semitones =
        static_cast<double>(note - center_note) + static_cast<double>(fine + center_fine) / 128.0;
    return std::exp2(semitones / 12.0);
}

double ps1_note_speed(std::int8_t center_note, std::int8_t center_fine, int note, int fine) {
    const bool ps1 = center_note >= 0;
    const int centre = ps1 ? center_note : -center_note;
    const double speed = note_speed(centre, static_cast<std::uint8_t>(center_fine), note, fine);
    return ps1 ? speed * 44100.0 / 48000.0 : speed;
}

std::pair<std::int16_t, std::int16_t> pitch_bend(
    const assets::Tone& tone, int bend, int modulation, int note, int fine
) {
    const int base = (note << 7) + fine + modulation;
    const int v = bend >= 0 ? tone.bend_up * (bend << 7) / 0x7fff + base
                            : tone.bend_down * (bend << 7) / 0x8000 + base;
    return {static_cast<std::int16_t>(v / 128), static_cast<std::int16_t>(v % 128)};
}

float tone_speed(const assets::Tone& tone, int bend, int modulation) {
    const auto [n, f] = pitch_bend(tone, bend, modulation, 60, 0);
    return static_cast<float>(ps1_note_speed(tone.center_note, tone.center_fine, n, f));
}

}  // namespace openrac::audio
