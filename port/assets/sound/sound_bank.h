// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/sound_bank.rs:
// ISC License, Copyright (c) 2026 ReRAC contributors.
//
// 989snd sound banks: the file the games' sound library loads into the
// sound processor's memory. The file is `u32 type = 3, u32 chunk count = 2,
// {u32 offset, u32 size}[2]`; chunk 0 is an SFX block ("SBlk": the sounds and
// their grain scripts), chunk 1 the raw SPU ADPCM the tones point into
// (adpcm.h). The layout was read by ReRAC from Ratchet & Clank (NTSC-U) and
// checked on every level's bank; the grain types are those OpenGOAL's 989snd
// (game/sound/989snd) names for Jak's build of the same library.
//
// Only version 1 blocks (12-byte sound records, 0x28-byte grains) are read:
// that is what Ratchet & Clank uses. Whether the later games' builds of
// 989snd write the same version is not known yet; another version is an
// error, not a guess.

#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include "assets/bytes.h"
#include "assets/sound/adpcm.h"

namespace openrac::assets {

// The SBlk header (block-relative). Its length varies (0x3c on the levels,
// 0x34 on the global bank in RAC1), so the sounds are always found through
// `first_sound`.
struct SfxBlockHeader {
    u32 version = 0;
    u32 flags = 0;    // 0x100 names, 0x200 user data (neither set in RAC1)
    u32 bank_id = 0;  // four characters ("DAW\0" on RAC1's Novalis) or 0
    s8 bank_number = 0;
    s16 sound_count = 0;
    s16 grain_count = 0;
    s16 vag_count = 0;
    u32 first_sound = 0;  // block-relative offset of the sound records
    u32 first_grain = 0;  // block-relative offset of the grain records
    u32 vags_in_sound_ram = 0;
    u32 vag_data_size = 0;
    u32 sound_ram_size = 0;
    u32 next_block = 0;
};

// One sound (12 bytes at first_sound + 12 * i).
struct SfxSound {
    s8 volume = 0;        // 0..127: play volume = volume * requested volume >> 10
    s8 volume_group = 0;  // the master-volume group the sound plays in
    s16 pan = 0;          // degrees
    s8 instance_limit = 0;
    u16 flags = 0;                // bit 0 looped; 2 solo; 8, 0x10, 0x20 instance-limit modes
    u32 first_grain_offset = 0;   // byte offset of its first grain from first_grain
    std::size_t first_grain = 0;  // index into SoundBank::grains
    std::size_t grain_count = 0;

    bool looped() const { return (flags & 1) != 0; }
};

// 989snd grain types (OpenGOAL's names; RAC1 uses 1, 4 and 20..43).
enum class GrainType : u32 {
    Null = 0,
    Tone = 1,
    XrefId = 2,
    XrefNum = 3,
    LfoSettings = 4,
    StartChildSound = 5,
    StopChildSound = 6,
    PluginMessage = 7,
    Branch = 8,
    Tone2 = 9,
    ControlNull = 20,
    LoopStart = 21,
    LoopEnd = 22,
    LoopContinue = 23,
    Stop = 24,
    RandPlay = 25,
    RandDelay = 26,
    RandPitchBend = 27,
    PitchBend = 28,
    AddPitchBend = 29,
    SetRegister = 30,
    SetRegisterRand = 31,
    IncRegister = 32,
    DecRegister = 33,
    TestRegister = 34,
    Marker = 35,
    GotoMarker = 36,
    GotoRandomMarker = 37,
    WaitForAllVoices = 38,
    PlayCycle = 39,
    AddRegister = 40,
    KeyOffVoices = 41,
    KillVoices = 42,
    OnStopMarker = 43,
    CopyRegister = 44,
};

// Tone flags.
inline constexpr u16 kToneToReverb = 0x01;
inline constexpr u16 kToneNoise = 0x08;
inline constexpr u16 kToneReverbOnly = 0x10;

// The parameters of a tone grain (types 1 and 9).
struct Tone {
    s8 priority = 0;  // voice-steal priority
    // 0..127; -1..-4 a sound register, -5 random, -6 and below a global register.
    s8 volume = 0;
    // Negative: a sample at the console's rate (the note is -center_note);
    // zero or more: a PS1-style sample, scaled by 44100 / 48000.
    s8 center_note = 0;
    s8 center_fine = 0;
    s16 pan = 0;  // degrees; negative values select registers as `volume`
    s8 map_low = 0;
    s8 map_high = 0;
    s8 bend_down = 0;  // pitch-bend range in semitones
    s8 bend_up = 0;
    u16 adsr1 = 0;
    u16 adsr2 = 0;
    u16 flags = 0;          // kTone*
    u32 sample_offset = 0;  // byte offset into the sample chunk
    u32 reserved = 0;
};

// The parameters of an LFO grain (type 4).
struct LfoSettings {
    u8 which = 0;
    u8 target = 0;  // 1 volume, 2 pan, 3 pitch modulation, 4 pitch bend
    u8 target_extra = 0;
    u8 shape = 0;  // 1 sine, 2 square, 3 triangle, 4 saw, 5 random
    u16 duty_cycle = 0;
    u16 depth = 0;
    u16 flags = 0;  // 1 invert, 2 random start
    u16 start_offset = 0;
    u32 step_size = 0;
};

// The parameters of the child-sound and branch grains (types 5, 6, 8).
struct ChildSoundParams {
    s32 volume = 0;
    s32 pan = 0;
    std::array<s8, 4> registers{};
    s32 sound_id = 0;
};

// A version 1 grain (0x28 bytes): `u32 type, s32 delay (240 Hz ticks),
// 32 bytes of parameters`.
struct Grain {
    GrainType type = GrainType::Null;
    s32 delay = 0;
    std::array<u8, 32> data{};

    bool is_tone() const { return type == GrainType::Tone || type == GrainType::Tone2; }

    Tone tone() const;
    LfoSettings lfo() const;
    ChildSoundParams child_sound() const;

    // The control grains' (20..44) `s16 param[4]`: version 1 grains store them
    // as 16-bit values.
    std::array<s16, 4> params() const;

    // RandDelay: version 1 grains store the modulus itself (rand() % amount).
    s32 rand_delay_amount() const;
};

struct SoundBank {
    u32 file_type = 0;
    std::array<std::pair<u32, u32>, 2> chunks{};  // (offset, size) of the block and the samples
    SfxBlockHeader header;
    std::vector<SfxSound> sounds;
    std::vector<Grain> grains;  // every sound's grains, in sound order
    // The distinct samples the tones use, by ascending offset.
    std::vector<SampleExtent> sample_extents;
    std::vector<u8> samples;  // chunk 1: raw SPU ADPCM

    // The index into sample_extents of the sample at `offset`.
    std::optional<std::size_t> extent_at(u32 offset) const;

    std::span<const Grain> sound_grains(std::size_t sound) const;
};

// Reads a bank file (RAC1: the level data's sound_bank lump, or the global
// one). Throws AssetError on anything else.
SoundBank parse_sound_bank(ByteView bytes);

}  // namespace openrac::assets
