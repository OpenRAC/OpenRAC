// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Archive primitives shared by the package reader, writer and source-key codec.
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace openrac::vkgl::package_detail {
// Archive identity and bounded block sizes, in bytes; all integers use little-endian words.
inline constexpr std::string_view kMagic = "ORSHDR02";
inline constexpr std::size_t kWordBytes = 8;
inline constexpr std::size_t kChunkBytes = std::size_t{16} * 1024;
inline constexpr std::size_t kMaxShaderBytes = std::size_t{16} * 1024 * 1024;
// Each encoded program has vertex, fragment and compute vectors, followed by reflection.
inline constexpr std::size_t kShaderStages = 3;
inline constexpr std::size_t kProgramSections = kShaderStages + 1;
// FNV-1a's 64-bit offset basis and prime, shared by source keys and compressed-block checksums.
inline constexpr std::uint64_t kBasis = 0xCBF29CE484222325ull;
inline constexpr std::uint64_t kPrime = 0x100000001B3ull;

// Appends an archive word without depending on the host byte order.
inline void number(std::vector<char>& out, std::uint64_t value) {
    for (unsigned n = 0; n < kWordBytes; ++n) {
        out.push_back(static_cast<char>(value >> (n * 8)));
    }
}

// Computes the checksum used to validate compressed blocks and find deduplication candidates.
inline std::uint64_t hash(std::span<const char> bytes) {
    std::uint64_t value = kBasis;

    for (unsigned char byte : bytes) {
        value = (value ^ byte) * kPrime;
    }

    return value;
}

// A bounded cursor borrowing the archive for one synchronous load.
class Reader {
public:
    // Borrows bytes owned by the caller of ShaderPackage::load.
    explicit Reader(std::span<const char> bytes) : m_bytes(bytes) {}

    // Returns the next word, making validity sticky when the input is truncated.
    std::uint64_t number() {
        const auto word = take(kWordBytes);
        std::uint64_t value = 0;

        for (unsigned n = 0; n < word.size(); ++n) {
            value |= std::uint64_t(static_cast<unsigned char>(word[n])) << (n * 8);
        }

        return value;
    }

    // Returns a borrowed range, or marks the reader invalid without advancing beyond the input.
    std::span<const char> take(std::uint64_t count) {
        if (!m_valid || count > remaining()) {
            m_valid = false;
            return {};
        }

        const auto out = m_bytes.subspan(m_at, static_cast<std::size_t>(count));
        m_at += static_cast<std::size_t>(count);
        return out;
    }

    // Returns the unread byte count; the cursor never passes the input's end.
    std::size_t remaining() const { return m_bytes.size() - m_at; }

    // Returns whether every requested range existed.
    bool valid() const { return m_valid; }

    // Returns whether the whole input was consumed without truncation.
    bool done() const { return m_valid && remaining() == 0; }

private:
    std::span<const char> m_bytes;  // Borrowed until load returns.
    std::size_t m_at = 0;           // Next unread archive byte.
    bool m_valid = true;            // False after the first truncated range.
};
}  // namespace openrac::vkgl::package_detail
