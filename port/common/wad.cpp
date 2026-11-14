// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "common/wad.h"

#include <algorithm>
#include <cstring>

namespace openrac::wad {

namespace {

std::uint32_t read_u32(std::span<const std::uint8_t> data, std::size_t at) {
    return static_cast<std::uint32_t>(data[at]) | static_cast<std::uint32_t>(data[at + 1]) << 8
           | static_cast<std::uint32_t>(data[at + 2]) << 16
           | static_cast<std::uint32_t>(data[at + 3]) << 24;
}

}  // namespace

std::optional<std::size_t> stream_size(std::span<const std::uint8_t> data) {
    if (data.size() < kHeaderBytes || data[0] != 'W' || data[1] != 'A' || data[2] != 'D') {
        return std::nullopt;
    }
    const std::size_t size = read_u32(data, 3);
    if (size < kHeaderBytes) {
        return std::nullopt;
    }
    return size;
}

bool looks_like_stream(std::span<const std::uint8_t> data) {
    const auto size = stream_size(data);
    return size && *size <= data.size();
}

std::optional<std::vector<std::uint8_t>> decompress(
    std::span<const std::uint8_t> data, std::size_t limit, std::size_t enough
) {
    const auto size = stream_size(data);
    if (!size || *size > data.size()) {
        return std::nullopt;
    }
    const std::size_t end = *size;
    std::size_t pos = kHeaderBytes;
    std::vector<std::uint8_t> out;
    // Most streams grow to two or three times their size: fewer moves of a large stream.
    out.reserve(std::min({limit, enough, end * 3}));
    bool bad = false;

    const auto byte = [&]() -> std::uint32_t {
        if (pos >= end) {
            bad = true;
            return 0;
        }
        return data[pos++];
    };
    const auto literal = [&](std::size_t count) {
        if (bad || pos + count > end || out.size() + count > limit) {
            bad = true;
            return;
        }
        out.insert(
            out.end(),
            data.begin() + static_cast<std::ptrdiff_t>(pos),
            data.begin() + static_cast<std::ptrdiff_t>(pos + count)
        );
        pos += count;
    };

    if (pos < end && data[pos] > 0x11) {
        literal(byte() - 0x11);
    }
    // The game traps on a literal run that follows literals.
    bool needs_match = !out.empty();
    while (pos < end && !bad && out.size() < enough) {
        const std::uint32_t tag = byte();
        std::size_t distance = 0;
        std::size_t count = 0;
        if (tag < 0x10) {
            if (needs_match) {
                return std::nullopt;
            }
            literal(tag != 0 ? tag + 3 : byte() + 18);
            needs_match = true;
            continue;
        }
        if (tag >= 0x40) {
            distance = 1 + ((tag >> 2) & 7) + byte() * 8;
            count = (tag >> 5) + 1;
        } else if (tag >= 0x20) {
            count = ((tag & 31) != 0 ? tag & 31 : byte() + 31) + 2;
            const std::uint32_t lo = byte();
            const std::uint32_t hi = byte();
            distance = 1 + (lo >> 2) + hi * 64;
        } else {
            count = (tag & 7) != 0 ? tag & 7 : byte() + 7;
            const std::uint32_t lo = byte();
            const std::uint32_t hi = byte();
            distance = (tag & 8) * 2048 + (lo >> 2) + hi * 64;
            if (distance != 0) {
                distance += 0x4000;
                count += 2;
            } else if (count == 1) {
                count = 0;
            } else {
                // Skip to the next 0x2000-byte block, which the game refills by DMA.
                pos = kHeaderBytes + (pos - kHeaderBytes + 0x1fff) / 0x2000 * 0x2000;
                if (pos > end) {
                    return std::nullopt;
                }
                needs_match = false;
                continue;
            }
        }
        if (bad || distance > out.size() || out.size() + count > limit) {
            return std::nullopt;
        }
        // A copy from bytes already out, which may overlap the bytes it makes (a repeat).
        const std::size_t from = out.size() - distance;
        out.resize(out.size() + count);
        std::uint8_t* const to = out.data() + from + distance;
        const std::uint8_t* const source = out.data() + from;
        if (distance >= count) {
            std::memcpy(to, source, count);
        } else {
            // A repeat: the bytes go out `distance` at a time, each from the ones before it.
            for (std::size_t done = 0; done < count; done += distance) {
                std::memcpy(to + done, source + done, std::min(distance, count - done));
            }
        }
        const std::size_t trailing = data[pos - 2] & 3;
        literal(trailing);
        needs_match = trailing != 0;
    }
    if (bad) {
        return std::nullopt;
    }
    return out;
}

}  // namespace openrac::wad
