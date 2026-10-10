// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/transition.rs:
// ISC License, Copyright (c) 2026 ReRAC contributors.
//
// RAC1's `transition` lump (global/transition.bin, WAD): what the flight
// between planets loads (NTSC-U EnterSpaceLoadingLoop 0x2a5868, the same code
// on all 19 overlays) and what its update (0x2a33b0) and draw (0x2a3b90)
// read. Decompressed, it is a header of u32 words and a data block at
// base = word 1 (0x9800 on NTSC-U); "data" offsets are relative to base,
// "lump" offsets to the start.
//
//   words 0, 2, 3      the GS upload: image at lump word 0, word 2 entries of
//                      the gs_ram layout at lump word 3
//   words 4, 5         3 moby classes, 8-word entries at lump word 5
//   words 8, 9, d, f   2 particle textures (the flight draws no particles)
//   words a, b, e      FX textures {palette, texture, width, height} at lump
//                      word b, the bank at data word e
//   words 10, 11       the moby chrome map (the same as every level's)
//   word 12            the sky block (6 shells)
//   word 13            the picture table: u32 count (133), then data offsets
//                      from the table: 0..18 the planet pictures (128x128
//                      PIF), 19 + 19 (language - 1) + level the area captions
//   words 14..18       the five flight variants (space-scene chunk lumps)
//   word 19            the flight's sound bank
//
// Pictures, textures and the sky are decoded by the geometry readers; this
// file gives their bytes.

#pragma once

#include <utility>
#include <vector>

#include "assets/bytes.h"

namespace openrac::assets::disc {

class TransitionLump {
public:
    static constexpr std::size_t kVariants = 5;  // 0..3 random, 4 the planet approach
    static constexpr std::size_t kLevels = 19;

    // From the lump's bytes, WAD-compressed or not.
    static TransitionLump parse(ByteView lump);

    const std::vector<u8>& bytes() const { return m_bytes; }

    std::size_t base() const { return m_base; }

    std::size_t word(std::size_t i) const;

    // Flight variant v's chunk lump (to the next variant or the sound bank).
    ByteView variant(std::size_t v) const;

    // The sky block (to the picture table).
    ByteView sky_block() const;

    // Picture k of the table: its bytes from its start to the end of the lump.
    ByteView picture(std::size_t k) const;

    // The destination's planet picture.
    ByteView planet_picture(s32 level) const;

    // The destination's area caption: entry 19 + 19 max(language - 1, 0) + level.
    ByteView caption(s32 language, s32 level) const;

    // One FX texture entry; negative fields mean an absent entry.
    struct FxEntry {
        s32 palette;
        s32 texture;
        s32 width;
        s32 height;
    };

    std::vector<FxEntry> fx_entries() const;

    // The FX bank the entries' offsets point into (to the end of the lump).
    ByteView fx_bank() const;

    // The flight's sound bank (to the end).
    ByteView sound_bank() const;

    // The moby classes: (o_class, data offset).
    std::vector<std::pair<s32, std::size_t>> classes() const;

    // The GS upload's image (lump word 0 to the data block).
    ByteView gs_image() const;

private:
    std::vector<u8> m_bytes;
    std::size_t m_base = 0;
};

}  // namespace openrac::assets::disc
