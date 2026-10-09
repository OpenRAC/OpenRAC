// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/sha1.rs:
// ISC License, Copyright (c) 2026 ReRAC contributors.
//
// SHA-1 (FIPS 180-4) and SHA-256, written from the published algorithms so
// the extractor needs no library. SHA-1 names builds and checks extracted
// files against their known hashes (games/*/game.json, the per-file tables);
// SHA-256 is the contents hash tools/extractor.py writes to buildinfo.json.
// Neither is used for anything security-relevant.

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace openrac::assets::disc {

using Sha1Digest = std::array<std::uint8_t, 20>;
using Sha256Digest = std::array<std::uint8_t, 32>;

class Sha1 {
public:
    void update(std::span<const std::uint8_t> data);
    void update(std::string_view text);
    // The digest of everything passed so far; the state is left unchanged.
    Sha1Digest finish() const;

private:
    std::array<std::uint32_t, 5> m_h{0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0};
    std::array<std::uint8_t, 64> m_block{};
    std::size_t m_fill = 0;
    std::uint64_t m_length = 0;
};

class Sha256 {
public:
    void update(std::span<const std::uint8_t> data);
    void update(std::string_view text);
    Sha256Digest finish() const;

private:
    std::array<std::uint32_t, 8> m_h{
        0x6a09e667,
        0xbb67ae85,
        0x3c6ef372,
        0xa54ff53a,
        0x510e527f,
        0x9b05688c,
        0x1f83d9ab,
        0x5be0cd19
    };
    std::array<std::uint8_t, 64> m_block{};
    std::size_t m_fill = 0;
    std::uint64_t m_length = 0;
};

Sha1Digest sha1(std::span<const std::uint8_t> data);
Sha256Digest sha256(std::span<const std::uint8_t> data);

// Lower-case hex.
std::string to_hex(std::span<const std::uint8_t> bytes);

// 40 hex digits of either case, or nothing.
std::optional<Sha1Digest> parse_sha1(std::string_view hex);

}  // namespace openrac::assets::disc
