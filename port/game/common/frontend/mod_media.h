// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mods' sounds (interface 1.12, openrac_api.sound_*), on the window's sound output: a .wav or
// .ogg file of a mod's decoded whole and handed to the sound engine (openrac/sound_engine.h), which
// mixes its voices with the game's. main.cpp gives these to the mods' loader (mods.h, Host).

#pragma once

#include <cstdint>
#include <filesystem>

namespace openrac::frontend {

// Decodes `file` (a .wav: PCM of 8, 16, 24 or 32 bits or 32-bit float, mono or stereo; or an
// .ogg: Vorbis) into the sound engine: its id, -1 when it cannot be read.
int mod_sound_load(const std::filesystem::path& file);
std::uint32_t mod_sound_play(int sound, float volume, float pitch, bool loop);
int mod_sound_set(std::uint32_t voice, float volume, float pitch);
void mod_sound_stop(std::uint32_t voice);

}  // namespace openrac::frontend
