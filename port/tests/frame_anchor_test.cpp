// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The frame C contract's cursor helper, using invented pointer values and a local cursor word.
// Expected addresses follow guest.h's direct RAM and alias ranges, not any game's data.

#include <array>
#include <cstdint>

#include "check.h"
#include "openrac/frame.h"

namespace {

// The anchor callback has no user pointer; each synchronous test lends it its output word.
std::uint32_t* g_anchor = nullptr;

// Captures the anchor produced for a readable cursor word without allocating its destination RAM.
std::uint32_t anchor_of(std::uint32_t pointer, std::uint32_t ram_size = GUEST_RAM_SIZE) {
    // Only the cursor word is read; its list pointer is never dereferenced by the helper.
    std::array<std::uint32_t, 2> words{0, pointer};
    std::uint32_t anchor = 0;
    g_anchor = &anchor;
    openrac_guest_base = reinterpret_cast<std::uint8_t*>(words.data());
    openrac_guest_ram_size = ram_size;
    openrac_frame_anchor_at(sizeof(std::uint32_t));
    openrac_guest_base = nullptr;
    g_anchor = nullptr;
    return anchor;
}

// Keeps a valid cursor in expanded RAM anchored rather than turning it into unconditional drawing.
void test_expanded_ram_cursor() {
    // The first quadword beyond normal RAM is valid only in the expanded arena.
    CHECK(anchor_of(GUEST_RAM_SIZE, GUEST_RAM_EXPANDED_SIZE) == GUEST_RAM_SIZE);
    CHECK(anchor_of(GUEST_RAM_SIZE) == UINT32_MAX);
}

// Normalizes the two RAM aliases to the same address the list walker uses.
void test_ram_aliases() {
    constexpr std::uint32_t kTag = 0x1000;
    CHECK(anchor_of(kTag) == kTag);
    CHECK(anchor_of(GUEST_RAM_UNCACHED + kTag) == kTag);
    CHECK(anchor_of(GUEST_RAM_UNCACHED_ACCEL + kTag) == kTag);
}

// Rejects null, unaligned and non-RAM list pointers without allowing unanchored drawing.
void test_unreachable_cursors() {
    constexpr std::uint32_t kTag = 0x1000;
    CHECK(anchor_of(0) == UINT32_MAX);
    CHECK(anchor_of(kTag + 1) == UINT32_MAX);
    CHECK(anchor_of(GUEST_GS_REGS) == UINT32_MAX);
    CHECK(anchor_of(UINT32_MAX) == UINT32_MAX);
    CHECK(anchor_of(GUEST_RAM_UNCACHED + GUEST_RAM_SIZE) == UINT32_MAX);
}

// Accepts the final complete tag, but not a tag crossing the active RAM boundary.
void test_terminal_tag_bounds() {
    // A list tag is one aligned quadword, 16 bytes.
    constexpr std::uint32_t kTagBytes = 16;
    CHECK(anchor_of(GUEST_RAM_SIZE - kTagBytes) == GUEST_RAM_SIZE - kTagBytes);
    CHECK(anchor_of(GUEST_RAM_SIZE - kTagBytes, GUEST_RAM_SIZE - 1) == UINT32_MAX);
}

// Rejects an absent cursor word without reading guest memory or clearing the command's anchor.
void test_absent_cursor_word() {
    std::uint32_t anchor = 0;
    g_anchor = &anchor;
    openrac_guest_base = nullptr;
    openrac_frame_anchor_at(0);
    CHECK(anchor == UINT32_MAX);
    g_anchor = nullptr;
}

}  // namespace

extern "C" {

// The contract reads these runtime globals; the test lends them only its local cursor storage.
std::uint8_t* openrac_guest_base = nullptr;
std::uint32_t openrac_guest_ram_size = GUEST_RAM_SIZE;

// Captures the C helper's synchronous callback without a frontend or a game.
void openrac_frame_anchor(gaddr list_cursor) {
    *g_anchor = list_cursor;
}

}  // extern "C"

// Runs the cursor helper checks without a runtime memory reservation.
int main() {
    test_expanded_ram_cursor();
    test_ram_aliases();
    test_unreachable_cursors();
    test_terminal_tag_bounds();
    test_absent_cursor_word();
    return openrac::test::result();
}
