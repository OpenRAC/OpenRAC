// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The games' WAD compression: a 16-byte header ("WAD", then the stream's size at +3) and LZO-like
// packets, which every Ratchet & Clank game decompresses the same way (rac1's func_0020C468). The
// same reading as editor/formats.py's wad(), which the editor uses on the disc's files; the
// interface texture runs (renderer/textures/interface_run.h) use it to find where a texture's
// bytes lie on the disc.

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace openrac::wad {

// The header's magic and size.
inline constexpr std::size_t kHeaderBytes = 16;

// The most a stream may decompress to, unless the caller says less.
inline constexpr std::size_t kDefaultLimit = std::size_t{64} << 20;

// Whether `data` starts like a WAD stream: the magic and a size that fits in `data`.
bool looks_like_stream(std::span<const std::uint8_t> data);

// The stream's size in bytes, from its header (16 or more), or nothing when it has no header.
std::optional<std::size_t> stream_size(std::span<const std::uint8_t> data);

// `data` (a stream from its header on; bytes after its size are not read) decompressed, or nothing
// when it is not a valid stream or would pass `limit` bytes.
std::optional<std::vector<std::uint8_t>> decompress(
    std::span<const std::uint8_t> data, std::size_t limit = kDefaultLimit
);

}  // namespace openrac::wad
