// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/water.rs
// and crates/rc-formats/src/font.rs: ISC License, Copyright (c) 2026 ReRAC contributors.
//
// The overlay's section walk and its reads by EE address.

#include "assets/world/overlay_memory.h"

#include "assets/world/known_games.h"

namespace openrac::assets {

OverlayMemory OverlayMemory::parse(Game game, ByteView lump) {
    require_world_layout(game, "level overlay");
    OverlayMemory memory;
    memory.m_entry = lump.u32_at(12);
    std::size_t at = 0;
    while (at + 0x10 <= lump.size()) {
        const u32 address = lump.u32_at(at);
        const u32 size = lump.u32_at(at + 4);
        const u32 type = lump.u32_at(at + 8);
        const u32 entry = lump.u32_at(at + 12);
        // The loader's own end: the first header with another entry point.
        if (entry != memory.m_entry) {
            break;
        }
        memory.m_sections.push_back(
            {address, type, entry, lump.sub(at + 0x10, size, "overlay section")}
        );
        at += 0x10 + size;
    }
    if (memory.m_sections.empty()) {
        fail("level overlay has no sections");
    }
    return memory;
}

const OverlaySection* OverlayMemory::find(u32 address, std::size_t length) const {
    const OverlaySection* found = nullptr;
    for (const OverlaySection& s : m_sections) {
        const u64 start = s.address;
        const u64 end = start + s.data.size();
        if (address >= start && u64{address} + length <= end) {
            if (s.type != 8) {
                return &s;
            }
            if (!found) {
                found = &s;
            }
        }
    }
    return found;
}

bool OverlayMemory::contains(u32 address, std::size_t length) const {
    return find(address, length) != nullptr;
}

ByteView OverlayMemory::read(u32 address, std::size_t length) const {
    const OverlaySection* s = find(address, length);
    if (!s) {
        fail("EE address {:#x} (+{:#x}) is not in the overlay", address, length);
    }
    return s->data.sub(address - s->address, length, "overlay read");
}

}  // namespace openrac::assets
