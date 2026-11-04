// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The port's own menu sounds: short, rounded "pops" made here (no file, nothing of the game's),
// mixed over the game's sound by the output (program/sound.cpp). The menu plays them while the game is
// paused, when no tick runs the game's voices.
//
// play() is called from the window's thread, mix() from the output's: a lock between them. A held
// arrow moves the focus 20 times a second; its cursor sounds are thinned out and each new one fades
// the one before it out, so a fast scroll is a steady, quiet tick, never a pile of sounds.

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace openrac::audio::ui_sounds {

enum class Sound : std::uint8_t {
    Cursor,   // the focus moved to another row
    Confirm,  // a row or button pressed
    Back,     // a page goes back
    Denied,   // a value already at its end
    Open,     // the menu opens
    Close,    // the menu closes
    Page,     // another tab or page
};

inline constexpr int kRate = 48000;

void play(Sound sound);

// Adds what plays now to `interleaved` (stereo, kRate), saturating.
void mix(std::span<std::int16_t> interleaved);

// Whether anything is still sounding (tests).
bool sounding();

// Stops everything at once and forgets the cursor's last time (tests).
void reset();

}  // namespace openrac::audio::ui_sounds
