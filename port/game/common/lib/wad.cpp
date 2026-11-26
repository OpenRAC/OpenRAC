// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The asset readers' WAD decompressor (assets/disc/wad.h), for the games' own
// code: the games decompress their level data in memory with a hand-written
// routine the port answers with this.

#include <cstdint>
#include <cstring>

#include "assets/disc/wad.h"
#include "common/log.h"

// Decompresses the WAD stream at src into dst (at most capacity bytes) and
// returns how many bytes it wrote; 0 when src holds no WAD stream or a
// malformed one.
extern "C" uint32_t openrac_lib_wad_decompress(const uint8_t* src, uint8_t* dst,
                                                uint32_t capacity) {
    using namespace openrac::assets;
    const ByteView header(src, disc::kWadHeaderSize);
    if (!disc::is_wad(header)) {
        return 0;
    }
    try {
        const ByteView stream(src, disc::wad_compressed_size(header));
        const std::vector<u8> out = disc::wad_decompress(stream);
        if (out.size() > capacity) {
            openrac::log::warn("a WAD output needs {} bytes but its buffer holds {}", out.size(), capacity);
            return 0;
        }
        std::memcpy(dst, out.data(), out.size());
        return static_cast<uint32_t>(out.size());
    } catch (const AssetError& error) {
        openrac::log::warn("a WAD stream the game decompresses is malformed: {}", error.what());
        return 0;
    }
}

// The games' own routine on the stream at src (wad_decompress_as_game): `readable` bytes may be
// read from src on, `before` bytes lie before dst and `capacity` after it. Sets *made to the
// routine's result and returns 1; returns 0 when the routine traps or the output does not fit.
extern "C" int openrac_lib_wad_decompress_as_game(
    const uint8_t* src,
    uint32_t readable,
    uint8_t* dst,
    uint32_t before,
    uint32_t capacity,
    uint32_t* made
) {
    using namespace openrac::assets;
    try {
        const disc::GameWadOutput out =
            disc::wad_decompress_as_game(ByteView(src, readable), ByteView(dst - before, before));
        if (out.bytes.size() > capacity) {
            openrac::log::warn(
                "a WAD output needs {} bytes but its buffer holds {}", out.bytes.size(), capacity
            );
            return 0;
        }
        std::memcpy(dst, out.bytes.data(), out.bytes.size());
        *made = static_cast<uint32_t>(out.size);
        return 1;
    } catch (const AssetError& error) {
        openrac::log::warn("a WAD stream the game decompresses is malformed: {}", error.what());
        return 0;
    }
}
