// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/tfrag.rs
// (AdGif, GsWrap, GsFilter, ee_log2): ISC License, Copyright (c) 2026 ReRAC contributors.
//
// GS A+D quadwords as the game stores them inside its geometry: texture
// state (TEX0, TEX1, CLAMP, MIPTBP) that the GIF writes straight into GS
// registers. The stored values are packed fields that the level loader
// rewrites at load; tfrag.h and shrub.h decode them. The quadword layout and
// the register fields are the PS2's.

#pragma once

#include "assets/bytes.h"

#include <array>
#include <bit>

namespace openrac::assets {

// One GIF A+D quadword (16 bytes).
struct AdGif {
    s32 data_lo = 0;  // 0x00: low word of the 64-bit register value
    s32 data_hi = 0;  // 0x04: high word
    u8 address = 0;   // 0x08: GS register address
    // 0x09: padding; bytes 0x0c..0x10 are the quadword's w lane, which some
    // geometry uses for its own purposes (tfrag TEX1: the UV bias 2048.0f;
    // shrubs: the block's GS-packet slot).
    std::array<u8, 7> pad{};

    u64 value() const {
        return static_cast<u64>(static_cast<u32>(data_hi)) << 32 | static_cast<u32>(data_lo);
    }

    // The w lane (bytes 0x0c..0x10) as a signed word.
    s32 w_lane() const {
        return static_cast<s32>(
            u32{pad[3]} | u32{pad[4]} << 8 | u32{pad[5]} << 16 | u32{pad[6]} << 24
        );
    }
};
static_assert(sizeof(AdGif) == 0x10);

// GS CLAMP_1 wrap mode (WMS / WMT).
enum class GsWrap : u8 {
    Repeat,
    Clamp,
    RegionClamp,
    RegionRepeat,
};

inline GsWrap gs_wrap(u64 bits) { return static_cast<GsWrap>(bits & 3); }

// GS TEX1_1 MMAG / MMIN filter. MMAG only uses the first two.
enum class GsFilter : u8 {
    Nearest,
    Linear,
    NearestMipmapNearest,
    NearestMipmapLinear,
    LinearMipmapNearest,
    LinearMipmapLinear,
};

inline GsFilter gs_filter(u64 bits) {
    const u64 v = bits & 7;
    return static_cast<GsFilter>(v > 5 ? 5 : v);
}

// The game's floor(log2 x) for x > 0, 30 - PLZCW(x) (a boot helper at NTSC-U
// 0x1f97a0); used for the TW / TH fields.
inline s64 ee_log2(s32 x) {
    const u32 v = x >= 0 ? static_cast<u32>(x) : ~static_cast<u32>(x);
    return 30 - (static_cast<s64>(std::countl_zero(v)) - 1);
}

// TEX1 K as the signed 12-bit raw value in 1/16 mip levels, from the stored
// low word.
inline s16 lod_k_raw(s32 tex1_lo) {
    return static_cast<s16>(static_cast<s16>(static_cast<u16>((static_cast<u32>(tex1_lo) & 0xfff) << 4)) >> 4);
}

// A 64-bit register field.
inline u64 field(u64 value, int shift, int width) {
    return (value >> shift) & ((u64{1} << width) - 1);
}

}  // namespace openrac::assets
