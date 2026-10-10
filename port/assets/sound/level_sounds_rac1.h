// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/sound_bank.rs:
// ISC License, Copyright (c) 2026 ReRAC contributors.
//
// What a Ratchet & Clank (RAC1) level says about its sounds, beside the bank:
// the game's sound definitions and the remap that turns their indices into
// bank sound ids (core index +0x70), and the two gameplay sections the sound
// code reads (sound instances, env sample points). Every layout here is
// RAC1's, read by ReRAC from NTSC-U (SCUS_971.99) and its level programs;
// the addresses in the comments are NTSC-U level01 addresses. The later
// games' layouts are not known yet, so there is no rac2..rac4 path.

#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <vector>

#include "assets/bytes.h"

namespace openrac::assets {

// The game's sound definition (0x20 bytes), for level sounds and for each
// moby class's sounds.
struct SoundDef {
    f32 near = 0;         // 0x00: full volume at or inside this distance
    f32 far = 0;          // 0x04: `volume_far` at or beyond this distance
    s32 volume_far = 0;   // 0x08 (0x400 = unity)
    s32 volume_near = 0;  // 0x0c
    s32 bend_low = 0;     // 0x10, 0x14: pitch-bend range, lo + rand() % (hi - lo) per play
    s32 bend_high = 0;
    u8 looped = 0;  // 0x18: must match the play's loop flag, else the play is refused
    // 0x19: bit 0 squared falloff, bit 1 no occlusion, bit 2 not halved above
    // water, bit 3 no underwater pitch drop.
    u8 flags = 0;
    // 0x1a: on the disc an index into the remap's map; after the load the
    // bank sound id (0xffff: none).
    u16 index = 0;
    u32 bank_handle = 0;  // 0x1c: written by the loader (0 on the disc)
};

static_assert(sizeof(SoundDef) == 0x20);

// One moby class's sounds.
struct ClassSounds {
    s32 o_class = 0;
    std::vector<u16> bank_ids;  // from the remap, in class sound order
    // The class blob's defs (header +0x0d count, +0x28 pointer) with `index`
    // the remapped id; empty when the class has no blob in the level.
    std::vector<SoundDef> defs;
    std::optional<u8> header_count;  // blob header +0x0d; none without a blob
};

struct LevelSounds {
    std::vector<SoundDef> level_defs;  // with `index` = the bank sound id
    std::vector<u16> map;              // bank sound id per level def index
    std::vector<ClassSounds> classes;  // in core-index class order
};

// A moby class of the level core, as the remap needs it.
struct SoundRemapClass {
    s32 o_class = 0;
    s32 blob_offset = 0;  // its blob in the core data; 0 or less: none in this level
};

// Reads the remap block at `remap_offset` of the core index (core header
// +0x70; index-relative `s16 defs_off, defs_count, map_off, map_count`, then
// one `{s16 offset, s16 count}` per moby class) and applies it as the loader
// does (NTSC-U `LoadLevelCoreData` 0x258128). `core_data` is the decompressed
// core data that holds the class blobs. A remap offset of 0 or less gives an
// empty result.
LevelSounds parse_level_sounds_rac1(
    ByteView core_index,
    s32 remap_offset,
    std::span<const SoundRemapClass> classes,
    ByteView core_data
);

// A hand item's class (a gadget: the wrench, the Swingshot, every weapon) has
// no blob in the level's class table, so its defs are empty after the remap;
// the loader parks the first min(count, 15) ids of its remap list and writes
// them into the gadget blob's defs when the gadget loads (NTSC-U 0x259788).
// This applies that for every (o_class, decompressed blob) given; classes
// that have a level blob are left alone.
struct GadgetBlob {
    s32 o_class = 0;
    ByteView blob;
};

void apply_gadget_sounds_rac1(LevelSounds& sounds, std::span<const GadgetBlob> gadgets);

// Who asks for a sound.
struct SoundOwner {
    bool level = true;  // a level def; else class sound `local` of `o_class`
    s32 o_class = 0;

    static SoundOwner of_level() { return {true, 0}; }

    static SoundOwner of_class(s32 o_class) { return {false, o_class}; }
};

// The bank sound id of (owner, local), or none (out of range, or 0xffff).
std::optional<u16> resolve_sound(const LevelSounds& sounds, SoundOwner owner, std::size_t local);

// The def a play of (owner, local) uses, or null.
const SoundDef* find_sound_def(const LevelSounds& sounds, SoundOwner owner, std::size_t local);

// The gameplay file's header pointers of the two sound sections.
inline constexpr std::size_t kSoundInstancesPointerRac1 = 0x0c;
inline constexpr std::size_t kEnvSamplePointsPointerRac1 = 0x88;

// A sound instance (0x90 bytes). Class: 0 sphere, 1 box volume, 2 box
// one-shot, 3 reverb box, 5 underwater loop, 6 music box.
struct SoundInstance {
    s16 o_class = 0;
    s16 m_class = 0;
    s32 pvar_index = 0;
    f32 range = 0;
    std::array<std::array<f32, 4>, 4> matrix{};   // row 3 is the position
    std::array<std::array<f32, 4>, 3> inverse{};  // world offset to box space
    std::array<f32, 3> rotation{};

    std::array<f32, 3> position() const { return {matrix[3][0], matrix[3][1], matrix[3][2]}; }

    // v.x * inverse[0] + v.y * inverse[1] + v.z * inverse[2]: the box-local
    // coordinates of a world-space offset from the centre.
    std::array<f32, 3> to_local(std::array<f32, 3> v) const;
};

// Section 0x0c: `s32 count, pad[3]`, then the records.
std::vector<SoundInstance> parse_sound_instances_rac1(ByteView gameplay);

// An env sample point (0x30 bytes, section 0x88): the fields the sound code reads.
struct EnvSamplePoint {
    std::array<f32, 3> position{};
    s32 reverb_depth = 0;    // +0x20
    u8 reverb_type = 0;      // +0x24: libsd's effect type (3 studio B, 4 studio C, 9 pipe)
    u8 reverb_delay = 0;     // +0x25
    u8 reverb_feedback = 0;  // +0x26
    u8 reverb_enable = 0;    // +0x27
    s32 music_track = 0;     // +0x28: the track the level starts near this point
};

std::vector<EnvSamplePoint> parse_env_sample_points_rac1(ByteView gameplay);

}  // namespace openrac::assets
