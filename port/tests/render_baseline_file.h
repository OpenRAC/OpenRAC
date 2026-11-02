// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Checked, ephemeral RGBA8 baselines passed from a CTest setup fixture to its shards.
// Readers require the exact build/device/settings identity and byte count; no saved goldens.

#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace openrac::test {

// FNV-1a seed shared by payload checks and streamed executable identities.
inline constexpr std::uint64_t kBaselineHashSeed = 14695981039346656037ull;

// Detects corrupted snapshot bytes without accepting a different baseline identity.
inline std::uint64_t baseline_checksum(
    std::span<const std::uint8_t> bytes, std::uint64_t hash = kBaselineHashSeed
) {
    constexpr std::uint64_t kPrime = 1099511628211ull;
    for (auto byte : bytes) {
        hash = (hash ^ byte) * kPrime;
    }
    return hash;
}

// Hashes the complete executable with bounded memory; a missing/unreadable file has no identity.
inline std::optional<std::uint64_t> baseline_executable_checksum(
    const std::filesystem::path& path
) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return std::nullopt;
    }
    constexpr std::size_t kChunk =
        64 * 1024;  // One bounded streaming buffer, not an executable copy.
    std::array<std::uint8_t, kChunk> bytes;
    auto hash = kBaselineHashSeed;
    while (in.read(reinterpret_cast<char*>(bytes.data()), kChunk) || in.gcount() > 0) {
        hash =
            baseline_checksum(std::span(bytes.data(), static_cast<std::size_t>(in.gcount())), hash);
    }
    // An I/O error must not produce a usable identity for a partially read executable.
    if (!in.eof()) {
        return std::nullopt;
    }
    return hash;
}

// Writes an ephemeral file only after all producer comparisons have succeeded.
inline bool write_render_baseline(
    const std::filesystem::path& path,
    const std::string& identity,
    std::span<const std::uint8_t> bytes
) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << identity << '\n' << bytes.size() << '\n' << baseline_checksum(bytes) << '\n';
    out.write(
        reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())
    );
    out.close();
    return !out.fail();
}

// Rejects stale settings, wrong extents, corruption, truncation and trailing data before use.
inline std::vector<std::uint8_t> read_render_baseline(
    const std::filesystem::path& path, const std::string& identity, std::size_t expected_bytes
) {
    std::ifstream in(path, std::ios::binary);
    std::string actual;
    std::size_t size = 0;
    std::uint64_t checksum = 0;
    std::getline(in, actual);
    in >> size >> checksum;
    // The producer's identity and declared shape must match before allocating any payload.
    if (actual != identity || size != expected_bytes || in.get() != '\n') {
        return {};
    }
    std::vector<std::uint8_t> bytes(size);
    in.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(size));
    // A baseline is complete only when the entire checked payload was written successfully.
    if (!in || in.peek() != std::char_traits<char>::eof() || baseline_checksum(bytes) != checksum) {
        return {};
    }
    return bytes;
}

}  // namespace openrac::test
