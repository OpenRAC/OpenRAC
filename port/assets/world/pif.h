// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/pif.rs:
// ISC License, Copyright (c) 2026 ReRAC contributors.
//
// PIF images ("2FIP"): the pictures the menus stream from the global lumps
// (item images, help screenshots, the planets, the in-game maps, ...), each
// file WAD-compressed, and the spaceships' textures. Layout, as the texture
// setup (NTSC-U 0x259a38) and the map composer (0x259b78) read it: +0 "2FIP",
// +4 u32 size, +8 u32 width, +0xc u32 height, +0x10 u32 format (0x13: PSMT8
// with a CT32 palette), +0x20 the 256-entry palette (0x400 bytes, CSM1 order),
// +0x420 the 8-bit pixels, row major. The GS TEX0 the game builds: TW, TH the
// log2 of the size, PSM 0x13, CPSM 0, CSM 0. An in-game map file is a set of
// PIFs, read one at a time by offset.

#pragma once

#include <cstddef>

#include "assets/bytes.h"
#include "assets/image.h"
#include "assets/version.h"

namespace openrac::assets {

inline constexpr u32 kPifFormatPsmt8 = 0x13;
inline constexpr std::size_t kPifPaletteOffset = 0x20;
inline constexpr std::size_t kPifPixelsOffset = 0x420;

// One PIF, borrowed from its file.
struct PifImage {
    u32 width = 0;
    u32 height = 0;
    u32 format = 0;
    ByteView palette;  // 0x400 bytes
    ByteView pixels;   // width * height indices

    IndexedImage image() const { return {width, height, pixels, palette}; }
};

// The PIF at byte `at` of a decompressed lump.
PifImage read_pif(Game game, ByteView bytes, std::size_t at = 0);

}  // namespace openrac::assets
