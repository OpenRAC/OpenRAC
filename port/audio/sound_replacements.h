// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The sounds packs replace: audio/<key>.wav in the stacked mods folder (--mods), as
// editor/AUDIO_MODDING.md describes. A sound is known by its key, a hash of what it is (editor/audio.py
// works out the same ones): the same effect is in many levels under other numbers, and most of the
// game's sound is the same on both discs, so one file reaches a sound everywhere and on either disc.
// Nothing here runs without a mods folder.

#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <set>
#include <span>
#include <utility>

#include "assets/sound/sound_bank.h"
#include "audio/sound_player.h"

namespace openrac::audio {

using SoundKey = std::uint64_t;

// How much of a stream its key is made from: recordings of one line in several languages can start
// alike for many sectors, and reading a track's whole length would hold up the game.
inline constexpr std::size_t kStreamKeyBytes = std::size_t{1} << 20;

// The key of the stream (a VAG file on the disc) `start` begins with, given at least its first
// kStreamKeyBytes or all of it: its header says how long it is, and what follows is not part of it.
SoundKey stream_key(std::span<const std::uint8_t> start);

// The key of one sound of a bank: for each of its grains, its type and delay, then for a tone its
// settings and the bytes of its sample, for any other grain its parameters.
SoundKey sound_key(const assets::SoundBank& bank, std::size_t sound);

class SoundReplacements {
public:
    SoundReplacements() = default;

    // The WAV files of `mods`/audio named after a key; any other file there is left alone.
    static SoundReplacements read(const std::filesystem::path& mods);

    bool empty() const { return m_keys.empty(); }

    // The new samples for the sounds of `bank`. A file that cannot be used is logged, and its sound
    // stays the game's own.
    BankReplacements bank(const assets::SoundBank& bank) const;

    // The file to play in place of the stream `start` begins with (as stream_key takes it); empty
    // for its own.
    std::filesystem::path stream(std::span<const std::uint8_t> start) const;

private:
    // The new sample of the sound with this key, encoded once: a level's bank is loaded each time
    // the level is, and again by a warm save state. Null when its file cannot be used.
    SampleData sample(SoundKey key, bool loop) const;

    std::filesystem::path file(SoundKey key) const;

    std::filesystem::path m_folder;
    std::set<SoundKey> m_keys;
    mutable std::map<std::pair<SoundKey, bool>, SampleData> m_samples;
};

}  // namespace openrac::audio
