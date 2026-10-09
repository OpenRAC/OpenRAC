// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/vag.rs:
// ISC License, Copyright (c) 2026 ReRAC contributors.
//
// VAG files: a 0x30-byte big-endian header ("VAGp") and one mono SPU ADPCM
// body (adpcm.h). In Ratchet & Clank (NTSC-U, checked on the disc by ReRAC)
// they hold the level music (44100 Hz) and the scene speech (44056 Hz); the
// last two frames carry the flags 1 (end) then 7 (padding). The header is
// the console's usual one, so other games' VAGs read the same way; what they
// hold there is not known yet.

#pragma once

#include <cstddef>
#include <string>

#include "assets/bytes.h"

namespace openrac::assets {

inline constexpr std::size_t kVagHeaderSize = 0x30;

struct VagHeader {
    u32 version = 0;
    u32 data_size = 0;    // body bytes after the header
    u32 sample_rate = 0;  // Hz
    std::string name;     // up to 16 characters, e.g. "L01_Enemy_Loop"
};

struct VagFile {
    VagHeader header;
    // The body: `data_size` bytes, clipped to the file.
    ByteView body;
};

// Throws AssetError when `bytes` is not a VAGp file.
VagFile parse_vag(ByteView bytes);

}  // namespace openrac::assets
