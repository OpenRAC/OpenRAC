// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/cameras.rs:
// ISC License, Copyright (c) 2026 ReRAC contributors.
//
// A level's camera records (gameplay section 0x08) and their pvar blocks: the
// data of the level camera system (ReRAC's docs/plan/player_controller.md
// section 15). Addresses are NTSC-U.
//
// The section is `s32 count, pad[3]`, then count 0x20-byte records {s32 class,
// f32 position[3], f32 rotation[3], s32 pvar}. The loader (level01 0x255958)
// copies them reordered to {position, class, rotation, pvar} at 0x15ef50, and
// once the mobys' pvars are in place each record's pvar word becomes the
// address of its block's copy (0 for -1); the pointer fixups apply to it as to
// a moby's. Which classes a level has is its overlay's camera class table
// (one entry per class: class, activate, init, update, pre; -1 ends it).
//
// Every class's block starts with the same header (CameraHeader). The classes
// ReRAC has read further have their own layouts here: 3 (the rail camera), 14
// (the side view), 17 (follow-camera tweak regions), 18 (moby focus regions)
// and 23 (placed views). What each class does with them is game code.

#pragma once

#include <array>
#include <functional>
#include <optional>
#include <vector>

#include "assets/bytes.h"
#include "assets/world/gameplay.h"

namespace openrac::assets {

inline constexpr std::size_t kCameraRecordSize = 0x20;
// The runtime has 48 camera slots.
inline constexpr s32 kCameraSlots = 48;

struct CameraRecord {
    s32 class_id = 0;                 // +0x00 (runtime +0x0c, read as s16)
    std::array<float, 3> position{};  // +0x04 (runtime +0x00)
    // +0x10 (runtime +0x10); class 17 reads rotation.z as its region's facing.
    std::array<float, 3> rotation{};
    s32 pvar_index = 0;  // +0x1c, -1 for none

    bool operator==(const CameraRecord&) const = default;
};

// The records in file order (none when the section is absent).
std::vector<CameraRecord> read_camera_records(const GameplayFile& file);

struct LevelCamera {
    CameraRecord record;
    std::optional<std::vector<u8>> pvar;  // the loader's copy of the block
};

std::vector<LevelCamera> read_level_cameras(const GameplayFile& file);

// The loader's moby-link fixups on the cameras' blocks, matched by pvar index:
// each listed s32 is a gameplay instance index and becomes that instance's
// runtime moby index (`instance_to_moby`, -1 when it was not created);
// negative values are kept. Class 18's +0x28 is one.
void remap_camera_moby_links(
    std::vector<LevelCamera>& cameras,
    const GameplayFile& file,
    const std::function<std::optional<std::size_t>(std::size_t)>& instance_to_moby
);

// The first 0x20 bytes of every camera block.
struct CameraHeader {
    float w00 = 0;  // class-specific (class 17: the turn in degrees)
    // +0x08 sphere, +0x0c cuboid, +0x10 cylinder, +0x14 path (-1 none): the
    // shapes of the region test.
    s32 sphere = 0;
    s32 cuboid = 0;
    s32 cylinder = 0;
    s32 path = 0;
    float f18 = 0;    // 1.5 on most records; read by class 1 as its look height
    u8 priority = 0;  // +0x1c (0 never)
    // +0x1d: how a switch to this camera blends: 1 or 5 blend, 3 or 6 copy the
    // pose, else a cut.
    u8 blend = 0;
    u8 b1e = 0;
    // +0x1f: when the camera activates: 0 always, 1 or 2 when entered, 4 the
    // hero in the cuboid, 7 the hero's camera mode equals the class, else only
    // through the class's own hook.
    u8 activation = 0;

    // Nothing when the block is shorter than 0x20.
    static std::optional<CameraHeader> read(ByteView pvar);
};

// Class 17 (0x60 bytes): a region that retunes the follow camera while Ratchet
// is in it. Run-time words start as the file has them.
struct CameraRegionTweak {
    CameraHeader header;
    float turn = 0;          // +0x00: degrees a tick toward the region's facing (0 none)
    float tolerance = 0;     // +0x20: degrees (0 always)
    float distance = 0;      // +0x24 (0 leaves the follow camera's)
    float pivot_height = 0;  // +0x28
    s16 mode = 0;            // +0x2c: 0..11
    s16 counter = 0;         // +0x2e: ticks inside (run time)
    float look_height = 0;   // +0x30
    s16 leash = 0;           // +0x34: 0 turns the leash off
    s16 facing = 0;          // +0x36: only while the camera faces within 80 degrees
    float spring_k = 0;      // +0x38 / +0x3c: the horizontal spring eased in over 120 ticks
    float spring_d = 0;
    float from_k = 0;  // +0x40 / +0x44: the springs captured on entry (run time)
    float from_d = 0;
    s16 once = 0;  // +0x48
    s16 left = 0;  // +0x4a (run time)
    s32 cuboid2 = 0;
    float pitch = 0;   // +0x50: a scripted pitch in degrees (0 none)
    s16 no_pitch = 0;  // +0x54 / +0x56: the stick's pitch / yaw off
    s16 no_yaw = 0;

    static std::optional<CameraRegionTweak> read(ByteView pvar);
};

// Class 23 (0x60 bytes): a region that places the follow camera at the
// record's position while Ratchet is in it (the arrival spots).
struct CameraPlacedView {
    CameraHeader header;
    s32 counter = 0;  // +0x20: ticks inside (run time)
    s16 leash = 0;    // +0x24
    s16 done = 0;     // +0x26 (run time)
    float pitch = 0;  // +0x28: degrees (0 none)
    // +0x2c: radians; look height = height - distance * tan(angle).
    float look_angle = 0;
    float f30 = 0;
    s32 still = 0;       // +0x34: only while the follow camera is still
    float distance = 0;  // +0x38 / +0x3c / +0x40: from the position on the first ticks (run time)
    float height = 0;
    float look_height = 0;
    s16 not_clank = 0;   // +0x44
    s16 clank_only = 0;  // +0x46

    static std::optional<CameraPlacedView> read(ByteView pvar);
};

// Class 18 (0x60 bytes): a region that turns the follow camera toward a moby
// (or along the record's facing) while Ratchet is in it.
struct CameraMobyFocus {
    CameraHeader header;
    float turn = 0;       // +0x00: degrees a tick at full strength
    s16 counter = 0;      // +0x20 (run time; -1 the moby is gone)
    u8 near_kind = 0;     // +0x22: 1 = the region is `radius` from the moby
    float radius = 0;     // +0x24
    s32 moby = 0;         // +0x28: a moby link, used when `group` is -1
    float max_pitch = 0;  // +0x2c: degrees (0 none)
    float max_angle = 0;  // +0x30: degrees (0 none)
    float distance = 0;   // +0x34
    float pivot_height = 0;
    s16 mode = 0;           // +0x3c: 1..7
    s16 counter2 = 0;       // +0x3e (run time)
    float yaw = 0;          // +0x40 (run time)
    s32 group = 0;          // +0x44: the moby group, its first live member; -1 the moby
    float look_height = 0;  // +0x4c
    s32 suppress = 0;       // +0x50

    static std::optional<CameraMobyFocus> read(ByteView pvar);
};

// Class 3 (0x40 bytes): the rail and slide camera, riding a camera path beside
// Ratchet's rail.
struct CameraRail {
    CameraHeader header;
    float ahead = 0;   // +0x00: the look-ahead along the path (0.5 .. 15)
    s32 path = 0;      // +0x20: the camera path (-1 never current)
    s32 rail = 0;      // +0x24: the grind path Ratchet must ride, -1 any
    s32 map_path = 0;  // +0x28 / +0x2c: mode 2's mapping paths
    s32 map_rail = 0;
    s32 mode = 0;     // +0x30: 0 parallel, 2 mapped piecewise, else the nearest path point
    s16 mapped = 0;   // +0x34 (run time)
    s16 flipped = 0;  // +0x36 (run time)
    float along = 0;  // +0x38: the place offset along the path

    // Nothing when the block is shorter than 0x3c.
    static std::optional<CameraRail> read(ByteView pvar);
};

// Class 14 (0x40 bytes): the side view, keeping the record's facing behind and
// above a smoothed Ratchet while his feet are in the header's cuboid.
struct CameraSideView {
    CameraHeader header;
    float distance = 0;  // +0x20: behind along the facing
    float height = 0;    // +0x24: above Ratchet

    static std::optional<CameraSideView> read(ByteView pvar);
};

}  // namespace openrac::assets
