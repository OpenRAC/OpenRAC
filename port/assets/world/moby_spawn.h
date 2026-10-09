// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/moby_spawn.rs:
// ISC License, Copyright (c) 2026 ReRAC contributors.
//
// Which placed mobys the level loader creates, and the player's ship: the
// loader's spawn test over the gameplay file's instances (NTSC-U level01
// 0x255958, the instance loop at 0x256890..0x256a58), the instance -> runtime
// moby map the pvar fixups need, and the `spaceships` files. Specification:
// ReRAC's docs/plan/moby_update_catalogue.md.
//
// What each class's update then does at load (hide, move, change sequence) is
// game behaviour and is not here: the decompilation supplies it.

#pragma once

#include <array>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <vector>

#include "assets/bytes.h"
#include "assets/version.h"
#include "assets/world/gameplay.h"
#include "assets/world/pif.h"

namespace openrac::assets {

// The ship classes the loader picks from by the ship index (0x160548 in every
// level overlay), and the second class of each spaceships file (0x160558),
// which the loader only registers.
inline constexpr std::array<s32, 3> kRac1ShipClasses{531, 532, 533};
inline constexpr std::array<s32, 3> kRac1ShipExtraClasses{535, 536, 537};

// The ship index (0x13e056) on a level's first visit in a story playthrough:
// DoSpaceTransition (level01 0x2a68f8) sets 0, then 1 when planet 8 is
// unlocked or the destination is above 7, then 2 when planet 14 is or it is
// above 13. Planets 8 and 14 unlock on finishing 7 and 13, so on first visits
// the destination decides; a revisit after the unlocks takes the later ship.
std::size_t rac1_first_visit_ship(u32 level);

// The table of contents' spaceships entry the loader streams for a ship index
// (entry k holds class 530 + k; entry 0, class 530, only Veldin 1 uses, in its
// core).
constexpr std::size_t rac1_spaceships_entry(std::size_t ship) {
    return ship + 1;
}

// One `spaceships` file: two moby classes and one texture each. A word header
// (+0 ship class, +4 ship texture list, +8 extra class, +0xc extra texture
// list) and the blocks in the order class, extra class, texture, extra
// texture. A texture list is `count 1, first 0x10`, then one PIF: the loader
// (level01 0x253a18) takes the palette at list + 0x30 and the indices at
// list + 0x430, which is where the PIF puts them.
struct SpaceshipFile {
    ByteView ship_class;   // up to the extra class
    ByteView extra_class;  // up to the ship texture
    PifImage ship_texture;   // 256 x 256 PSMT8
    PifImage extra_texture;  // 128 x 128 PSMT8
};

// Borrows the file's bytes.
SpaceshipFile read_spaceship_file(Game game, ByteView file);

// The save state the spawn test reads for the level being loaded. The default
// is a first visit on a new game (everything zero).
struct SpawnSave {
    // 0x15fc88[16]: the level's mission bytes, copied from the save; 0xff is
    // a finished mission.
    std::array<u8, 16> missions{};
    // 0x14c190 + level * 0x100: the persistent killed bits, bit id & 7 of
    // byte id >> 3.
    std::array<u8, 0x100> killed{};
    // 0x1ba950: this visit's death bits, as spawn ids.
    std::set<s32> visit_deaths;
    // 0x1bbb04[id]: non-zero never spawns again (a collected bolt writes
    // mission + 2).
    std::map<s32, u8> id_flags;
    // 0x14d590 + level * 0x100: the 64 spawner slots handed to instances with
    // spawn flag 0x10 (slot value = spawn id + 1, 0 free).
    std::array<s16, 64> spawner{};

    bool killed_bit(s32 id) const;

    // FUN_0029ab50: the slot already holding id + 1, else the highest free
    // one, which takes it, scanning 63 down to 0; -1 for id < 0 or no slot.
    s32 spawner_slot(s32 id);
};

// The loader's result for one record.
struct SpawnDecision {
    bool spawn = true;
    // moby+0xb1: 0xfe without spawn flags, 0xff with flags but not 0x10, else
    // the spawner slot.
    u8 b1 = 0xfe;
    // moby+0xb4: start_value, or start_value_done on the "mission done"
    // branch, halved ((v + 1) / 2) when that branch's death bit is set.
    s16 b4 = 0;
    s16 b6 = 0;  // moby+0xb6: start_value, or start_value_done when done

    bool operator==(const SpawnDecision&) const = default;
};

// The spawn test of one record, in the code's order: nothing is tested without
// flags; b1 is the spawner slot (flag 0x10) or 0xff; a non-zero id flag stops
// it; with flags & 3 (mission-gated) a finished mission needs flag 2 (b4/b6
// from start_value_done) and an open one flag 1 (b4 halved when killed);
// else flag 8 needs no death this visit; else flags & 0xc == 4 needs no
// persistent kill.
SpawnDecision spawn_decision(const MobyInstance& instance, SpawnSave& save);

// spawn_decision over the records in order (the order hands out the slots).
std::vector<SpawnDecision> spawn_decisions(std::span<const MobyInstance> instances, SpawnSave& save);

// 0x1acc00: instance index -> runtime moby index (the created instances before
// it), nothing for an instance that was not created.
std::vector<std::optional<std::size_t>> instance_to_moby(std::span<const SpawnDecision> decisions);

}  // namespace openrac::assets
