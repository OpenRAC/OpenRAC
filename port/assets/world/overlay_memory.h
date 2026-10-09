// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/water.rs
// (`Overlay`) and crates/rc-formats/src/font.rs (`parse_overlay_sections`): ISC License,
// Copyright (c) 2026 ReRAC contributors.
//
// A level's code overlay seen as the console's memory. The overlay lump is a
// run of sections, each a 16-byte header {u32 address, u32 size, u32 type,
// u32 entry} and `size` bytes the loader copies to `address`; the loader stops
// at the end of the lump or at the first header whose entry point differs from
// the first one (ReRAC's docs/formats/wad_layouts_rac1.md section 4). Several
// world tables (water strips, ripple patches, the sea, the font glyphs) live
// in the overlay's data and are read from it here by their EE address, so no
// byte of the disc is part of the source.

#pragma once

#include <cstddef>
#include <vector>

#include "assets/bytes.h"
#include "assets/version.h"

namespace openrac::assets {

struct OverlaySection {
    u32 address;  // where the loader copies it
    u32 type;     // the ELF section type as stored: 1 PROGBITS, 8 NOBITS (its zeros are stored)
    u32 entry;
    ByteView data;
};

// Borrows the overlay's bytes: the lump must outlive it.
class OverlayMemory {
public:
    // The sections of a level overlay lump (rac1's "ratchet executable" layout).
    static OverlayMemory parse(Game game, ByteView lump);

    const std::vector<OverlaySection>& sections() const { return m_sections; }

    u32 entry() const { return m_entry; }

    // `length` bytes at `address`, inside one section (a PROGBITS section is
    // preferred where sections overlap). Throws when no section holds them.
    ByteView read(u32 address, std::size_t length) const;

    bool contains(u32 address, std::size_t length) const;

    u32 u32_at(u32 address) const { return read(address, 4).u32_at(0); }

    s32 s32_at(u32 address) const { return read(address, 4).s32_at(0); }

    s16 s16_at(u32 address) const { return read(address, 2).s16_at(0); }

    u16 u16_at(u32 address) const { return read(address, 2).u16_at(0); }

    f32 f32_at(u32 address) const { return read(address, 4).f32_at(0); }

private:
    const OverlaySection* find(u32 address, std::size_t length) const;

    std::vector<OverlaySection> m_sections;
    u32 m_entry = 0;
};

}  // namespace openrac::assets
