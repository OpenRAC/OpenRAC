// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace openrac::lighting {
inline constexpr std::size_t kMaxProbeAssetBytes = 512ull * 1024 * 1024;
inline constexpr std::size_t kMaxEnvironmentAssetBytes = 1024ull * 1024 * 1024;

bool bake_lz4_available();
bool is_lz4_bake_asset(std::span<const std::uint8_t> bytes);
// Returns an empty vector when a frame would not reduce the file size.
std::vector<std::uint8_t> compress_bake_asset(std::span<const std::uint8_t> bytes);
std::vector<std::uint8_t> decompress_bake_asset(
    std::span<const std::uint8_t> frame, std::size_t max_bytes
);
std::vector<std::uint8_t> read_bake_asset(const std::filesystem::path& path, std::size_t max_bytes);
}  // namespace openrac::lighting
