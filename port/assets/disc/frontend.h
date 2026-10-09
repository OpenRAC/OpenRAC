// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/frontend.rs
// (spec: docs/plan/progression.md, "Boot -> title"): ISC License, Copyright (c) 2026 ReRAC
// contributors.
//
// RAC1's front end, what the boot program shows before any level: the title
// world lump (global/unknown_14e8.bin, table field 0x14e8, WAD) and the boot
// pictures in the IRX lump (global/irx.bin, field 0x12c0, WAD).
//
// The title world (NTSC-U transition_load_wad 0x1ea830) decompresses to a
// header of u32 words and a data block at base = word 1. Word 0x21 + base is
// the title logo, 256x128 PSMCT32; words 0x16 and 0x17 the FX texture entries
// ({palette, texture, width, height}), the bank at word 0x1a + base: FX 1..3
// the fonts, 4..8 "PRESS START" in five languages, the rest particles.
//
// The same lump is level data with its words in another order, loaded with
// the level loader's own routines: the GS upload (data word 0, word 2 entries
// at word 3), tfrags word 4, the sky word 5, the moby, tie and shrub classes
// words 6..0xb, the texture tables words 0xc..0x13, the particle and FX
// textures 0x14..0x17, the texture data 0x18, the particle and FX banks 0x19
// and 0x1a, the particle definitions 0x1b, the chrome map 0x1c and 0x1d, the
// gameplay block 0x1f, the space-scene chunks 0x20 and the logo 0x21.
// TitleWorld rebuilds a core index in the level layout over the same tables,
// so the level readers read the title world unchanged.
//
// The boot pictures (NTSC-U init_once 0x201650 decompresses the lump; startlevel
// 0x1e9658 draws them): {u32 offset, u32 size} pairs, each a WAD of a raw
// 512x416 PSMCT32 frame: +0x00 NTSC and +0x08 PAL the still shown while the
// title loads, +0x10 + 8 language "no memory card", +0x40 + 8 language
// "insufficient space"; the IOP modules from +0x70.
//
// The FX textures are decoded by the geometry readers; this file gives their
// entries and bytes.

#pragma once

#include <vector>

#include "assets/bytes.h"
#include "assets/disc/level.h"

namespace openrac::assets::disc {

inline constexpr u32 kBootPictureWidth = 512;
inline constexpr u32 kBootPictureHeight = 416;
inline constexpr u32 kTitleLogoWidth = 256;
inline constexpr u32 kTitleLogoHeight = 128;

// An RGBA image as the GS holds it (alpha 0..0x80).
struct GsImage {
    u32 width;
    u32 height;
    std::vector<u8> rgba;
};

struct FxTextureEntry {
    s32 palette;  // offsets into the FX bank
    s32 texture;
    s32 width;
    s32 height;

    bool present() const { return palette >= 0 && texture >= 0 && width > 0 && height > 0; }
};

struct TitleLump {
    std::vector<u8> bytes;  // decompressed
    GsImage logo;
    std::vector<FxTextureEntry> fx;
    std::size_t fx_bank;  // where the FX bank starts in `bytes`

    // From the lump's bytes, WAD-compressed or not.
    static TitleLump parse(ByteView lump);

    // "PRESS START" of `language` (the game's number): FX max(language - 1, 0) + 4.
    std::size_t press_start_index(u32 language) const;
};

enum class BootPictureKind : u8 {
    Still,    // the still while the title loads
    NoCard,   // no memory card
    NoSpace,  // not enough space on it
};

class BootPictures {
public:
    static BootPictures parse(ByteView lump);

    // `pal` selects the still's region; `language` the warnings' language.
    GsImage picture(BootPictureKind kind, bool pal, u32 language) const;

private:
    std::vector<u8> m_bytes;
};

struct TitleWorld {
    static constexpr std::size_t kIndexTables = 0x100;  // where the lump's tables start in `index`

    std::vector<u8> index;  // a core index in the level layout
    LevelCore core;
    std::vector<u8> data;      // the data block: the core data
    std::vector<u8> gs;        // the GS upload image
    std::vector<u8> gameplay;  // the gameplay block
    std::vector<u8> scene;     // the space-scene chunk lump

    static TitleWorld parse(ByteView lump);
};

}  // namespace openrac::assets::disc
