// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/vag.rs and
// crates/rc-game/src/audio/grain_vm.rs: ISC License, Copyright (c) 2026 ReRAC contributors.
//
// Pitch: the sound processor plays a voice at pitch / 0x1000 * 48000 Hz
// (0x1000 is 48 kHz; the hardware caps the step at 0x3fff). The sound
// library turns notes into that word with libsd's note table: twelve
// semitone ratios, then 128 steps of 1/128 semitone, 0x8000 * 2^(k/12) and
// 0x8000 * 2^(k/1536) truncated. ReRAC checked the formula against the 140
// values RAC1's libsd module carries; OpenGOAL's 989snd (989snd/util.cpp)
// has the same table. The integer steps below follow OpenGOAL's
// sceSdNote2Pitch and 989snd's PS1Note2Pitch and PitchBend; the arithmetic
// is libsd's, the same for every game that links it.

#pragma once

#include <array>
#include <cstdint>
#include <utility>

#include "assets/sound/sound_bank.h"

namespace openrac::audio {

// The 140-entry note table (computed, not copied).
const std::array<std::uint16_t, 140>& note_pitch_table();

// The pitch word that plays a sample recorded at center_note + center_fine /
// 128 (at 48 kHz) as note + fine / 128. C division truncating toward zero,
// step for step.
std::uint16_t note_to_pitch(
    std::uint16_t center_note, std::uint16_t center_fine, std::uint16_t note, std::int16_t fine
);

// 989snd's PS1Note2Pitch: a negative centre note is a sample at the
// console's rate (its note is -center_note); a non-negative one is a
// PS1-style 44.1 kHz sample, scaled by 44100 / 48000 after the lookup.
std::uint16_t ps1_note_to_pitch(
    std::int8_t center_note, std::int8_t center_fine, std::int16_t note, std::int16_t fine
);

// The pitch word of a stream recorded at `rate` Hz: rate * 0x1000 / 48000,
// truncated and capped at 0x3fff. Inferred by ReRAC: the stream player's own
// computation is not reversed.
std::uint16_t rate_to_pitch(std::uint32_t rate);

// 989snd's PitchBend(tone, bend, modulation, note, fine): v = note * 128 +
// fine + modulation, plus bend_up * (bend << 7) / 0x7fff for an upward bend
// or bend_down * (bend << 7) / 0x8000 for a downward one; returns
// (v / 128, v % 128).
std::pair<std::int16_t, std::int16_t> pitch_bend(
    const assets::Tone& tone, int bend, int modulation, int note, int fine
);

// The pitch of a tone at a sound's bend and modulation (block sounds start
// at note 60, fine 0).
std::uint16_t tone_pitch(const assets::Tone& tone, int bend, int modulation);

}  // namespace openrac::audio
