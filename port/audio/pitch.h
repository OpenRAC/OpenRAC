// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/vag.rs and
// crates/rc-game/src/audio/grain_vm.rs: ISC License, Copyright (c) 2026 ReRAC contributors.
//
// Pitch as the sound library asks for it: a tone's sample was recorded at its centre note (and a
// fine tuning in 1/128 semitones, added), and plays a note and fine step higher at
// 2^(semitones / 12) times its speed. A sample of a negative centre note is at the console's rate
// (48 kHz), whose note is -center_note; a non-negative one is a PS1-style 44.1 kHz sample, played
// 44100 / 48000 as fast (989snd's PS1Note2Pitch). The bend follows 989snd's PitchBend (OpenGOAL's
// game/sound/989snd/util.cpp). The result is a speed for the native mixer (mixer.h): 1 plays the
// sample at 48 kHz.

#pragma once

#include <cstdint>
#include <utility>

#include "assets/sound/sound_bank.h"

namespace openrac::audio {

// How fast a sample recorded at center_note + center_fine / 128 (at 48 kHz) plays note + fine / 128.
double note_speed(int center_note, int center_fine, int note, int fine);

// 989snd's PS1Note2Pitch as a speed: a negative centre note is a sample at the console's rate (its
// note is -center_note); a non-negative one is a 44.1 kHz sample.
double ps1_note_speed(std::int8_t center_note, std::int8_t center_fine, int note, int fine);

// 989snd's PitchBend(tone, bend, modulation, note, fine): v = note * 128 +
// fine + modulation, plus bend_up * (bend << 7) / 0x7fff for an upward bend
// or bend_down * (bend << 7) / 0x8000 for a downward one; returns
// (v / 128, v % 128).
std::pair<std::int16_t, std::int16_t> pitch_bend(
    const assets::Tone& tone, int bend, int modulation, int note, int fine
);

// The speed of a tone at a sound's bend and modulation (block sounds start at note 60, fine 0).
float tone_speed(const assets::Tone& tone, int bend, int modulation);

}  // namespace openrac::audio
