// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/texture.rs:
// ISC License, Copyright (c) 2026 ReRAC contributors.
//
// 8-bit indexed images as the games store them (PSMT8 with a 256-entry
// RGBA32 palette in CSM1 order), and their RGBA8 decode. Every textured
// format of every part reads its pixels through here.

#pragma once

#include <vector>

#include "assets/bytes.h"

namespace openrac::assets {

// An RGBA8 image, rows top-down.
struct RgbaImage {
    u32 width = 0;
    u32 height = 0;
    std::vector<u8> rgba;

    bool operator==(const RgbaImage&) const = default;
};

// CSM1 palette order to linear: within each 32-entry group the two middle
// 8-entry blocks swap (bits 3 and 4 of the index). Its own inverse.
constexpr u32 clut_index(u32 i) {
    return ((i >> 3) & 1) != ((i >> 4) & 1) ? i ^ 0x18 : i;
}

// GS alpha 0..0x80 to 0..0xff; 0x80 and above is opaque.
constexpr u8 scale_alpha(u8 a) {
    return a < 0x80 ? static_cast<u8>(a * 2) : u8{0xff};
}

enum class GsAlpha : u8 {
    Scaled,  // 0..0x80 doubled to 0..0xff, as a renderer wants it
    Raw,     // as stored: 0x80 is opaque, as the GS samples it
};

// `width * height` palette indices with a 1024-byte CSM1 palette.
RgbaImage decode_indexed8(
    ByteView indices, u32 width, u32 height, ByteView clut, GsAlpha alpha = GsAlpha::Scaled
);

// One stored 8-bit image: the indices and the palette as the disc holds them.
struct IndexedImage {
    u32 width = 0;
    u32 height = 0;
    ByteView indices;
    ByteView clut;

    RgbaImage decode(GsAlpha alpha = GsAlpha::Scaled) const {
        return decode_indexed8(indices, width, height, clut, alpha);
    }
};

}  // namespace openrac::assets
