// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/vif.rs:
// ISC License, Copyright (c) 2026 ReRAC contributors.
//
// VIF1 command lists, the console's way of feeding vector-unit memory. Level
// geometry (tfrags, shrubs, moby packets) is stored as these lists, so the
// readers walk them code by code: each code word with its inline payload
// (unpack data, STROW rows, ...). Framing only; what an unpack means is the
// caller's business. The command numbers are the PS2's, not a game's.

#pragma once

#include "assets/bytes.h"

#include <cstddef>
#include <vector>

namespace openrac::assets::vif {

// Command numbers (bits 30..24 of the code word) the readers meet.
constexpr u8 kNop = 0x00;
constexpr u8 kStcycl = 0x01;
constexpr u8 kOffset = 0x02;
constexpr u8 kBase = 0x03;
constexpr u8 kItop = 0x04;
constexpr u8 kStmod = 0x05;
constexpr u8 kMskpath3 = 0x06;
constexpr u8 kMark = 0x07;
constexpr u8 kFlushe = 0x10;
constexpr u8 kFlush = 0x11;
constexpr u8 kFlusha = 0x13;
constexpr u8 kMscal = 0x14;
constexpr u8 kMscalf = 0x15;
constexpr u8 kMscnt = 0x17;
constexpr u8 kStmask = 0x20;
constexpr u8 kStrow = 0x30;
constexpr u8 kStcol = 0x31;
constexpr u8 kMpg = 0x4a;
constexpr u8 kDirect = 0x50;
constexpr u8 kDirecthl = 0x51;

// One code with its payload.
struct Code {
    u8 cmd = 0;      // bits 30..24; the interrupt bit 31 is dropped
    u8 num = 0;      // bits 23..16 (0 means 256 for unpacks)
    u16 imm = 0;     // bits 15..0
    std::size_t offset = 0;  // byte offset of the code word in the list
    ByteView data;   // inline payload

    bool is_unpack() const { return (cmd & 0x60) == 0x60; }

    // 0..3 for 1..4 components.
    u8 vn() const { return static_cast<u8>((cmd >> 2) & 3); }

    // 0 = 32-bit, 1 = 16-bit, 2 = 8-bit, 3 = 5-bit elements.
    u8 vl() const { return static_cast<u8>(cmd & 3); }

    // Unpack USN bit: unsigned elements.
    bool usn() const { return ((imm >> 14) & 1) != 0; }

    // Unpack FLG bit: the address is relative to TOPS (the double buffer).
    bool flg() const { return (imm & 0x8000) != 0; }

    // Unpack destination quadword address.
    u16 addr() const { return static_cast<u16>(imm & 0x3ff); }

    // Unpack element count, NUM with 0 meaning 256.
    u32 count() const { return num == 0 ? 256u : num; }

    // Bytes per unpacked element in the list.
    u32 element_size() const { return ((32u >> vl()) * (vn() + 1u)) / 8u; }
};

// Every code of a list up to its end. Fewer than four trailing bytes are not
// a code word and are ignored; a payload running past the end is an error.
std::vector<Code> parse(ByteView list);

}  // namespace openrac::assets::vif
