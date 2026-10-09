// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/texture.rs
// (`clut_index`, `scale_alpha`, `decode_indexed8`) and crates/rc-formats/src/hud.rs
// (`decode_indexed8_raw`): ISC License, Copyright (c) 2026 ReRAC contributors.
//
// 8-bit indexed images with a 256-entry RGBA32 palette, the GS's PSMT8 with a
// CT32 CLUT: what the HUD banks and the menus' PIF pictures store. The palette
// is in the GS's CSM1 order, which swaps index bits 3 and 4.

#pragma once

#include <cstddef>
#include <vector>

#include "assets/bytes.h"

namespace openrac::assets {

// A borrowed indexed image: width * height indices and a 0x400-byte palette.
struct Psmt8Image {
    u32 width = 0;
    u32 height = 0;
    ByteView indices;
    ByteView palette;
};

// How a decoded pixel's alpha is given.
enum class GsAlpha : u8 {
    Raw,     // as stored: 0x80 is opaque, as the GS samples it
    Scaled,  // 0..0x80 doubled to 0..0xff (0x80 and above opaque)
};

struct Rgba8Pixels {
    u32 width = 0;
    u32 height = 0;
    std::vector<u8> rgba;  // row major, 4 bytes a pixel
};

// The palette entry an index reads in CSM1 order: bits 3 and 4 swapped.
constexpr u32 csm1_palette_slot(u32 index) {
    return ((index >> 3) & 1) != ((index >> 4) & 1) ? index ^ 0x18 : index;
}

// 0..0x80 to 0..0xff, the console's opaque 0x80 to 0xff.
constexpr u8 scale_gs_alpha(u8 a) {
    return a < 0x80 ? static_cast<u8>(a * 2) : u8{0xff};
}

Rgba8Pixels decode_psmt8(const Psmt8Image& image, GsAlpha alpha);

}  // namespace openrac::assets
