// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/wad.rs and
// docs/formats/disc_layout.md section 3: ISC License, Copyright (c) 2026 ReRAC contributors.
//
// Insomniac's "WAD" compression: a byte-oriented LZ77 stream behind a
// 16-byte header ("WAD", the compressed size including the header, nine free
// bytes). It has nothing to do with Doom's WAD archives. Every compressed lump
// of the disc uses it: a level's core data and gameplay files, HUD banks,
// scene chunks, the global lumps.
//
// The decoder follows ReRAC's, which decodes all 4674 streams of the NTSC-U
// disc (each level's decompressed core data has the size its core index
// records). The far-match displacement 0x4000 * (A + 1) is the form verified
// there; A = 1 is never seen on that disc.
//
// The encoder follows the format's rules as ReRAC's spec states them (3.5):
// two literal packets are never adjacent, 1 to 3 literal bytes ride in the
// packet before them (a dummy packet when there is none), no packet crosses
// a 0x2000-byte chunk of the stream (a pad packet and filler before it), and
// far matches never set A. Its output decodes with the decoder here; it is not
// byte-identical to Insomniac's compressor, and whether the game accepts it
// has not been tried.

#pragma once

#include <vector>

#include "assets/bytes.h"

namespace openrac::assets::disc {

inline constexpr std::size_t kWadHeaderSize = 0x10;

// The first bytes are a WAD header.
bool is_wad(ByteView bytes);

// The whole stream's size in bytes, header included. Throws when `bytes` is
// not a WAD stream.
u32 wad_compressed_size(ByteView bytes);

// Decompresses the stream at the start of `bytes` (bytes after its
// compressed size are ignored). Throws AssetError on a malformed stream.
std::vector<u8> wad_decompress(ByteView bytes);

// A WAD stream that wad_decompress turns back into `data`.
std::vector<u8> wad_compress(ByteView data);

}  // namespace openrac::assets::disc
