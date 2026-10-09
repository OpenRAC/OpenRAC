// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Based on ReRAC's research (https://github.com/re-rac/rerac,
// crates/rc-formats/src/moby_spawn.rs): ISC License, Copyright (c) 2026 ReRAC
// contributors.
//
// RAC1's `spaceships` files: the player's ship in each of its three forms,
// streamed by the level loader (NTSC-U level 1 0x253a18). Which ship a level
// shows, and which placed mobys the loader creates, is game behaviour the
// decompilation supplies.

#pragma once

#include <array>

#include "assets/bytes.h"
#include "assets/version.h"
#include "assets/world/pif.h"

namespace openrac::assets {

// The ship classes the loader picks from by the ship index (0x160548 in every
// level overlay), and the second class of each spaceships file (0x160558),
// which the loader only registers.
inline constexpr std::array<s32, 3> kRac1ShipClasses{531, 532, 533};
inline constexpr std::array<s32, 3> kRac1ShipExtraClasses{535, 536, 537};

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
    ByteView ship_class;     // up to the extra class
    ByteView extra_class;    // up to the ship texture
    PifImage ship_texture;   // 256 x 256 PSMT8
    PifImage extra_texture;  // 128 x 128 PSMT8
};

// Borrows the file's bytes.
SpaceshipFile read_spaceship_file(Game game, ByteView file);

}  // namespace openrac::assets
