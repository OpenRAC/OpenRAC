// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// HDR texture formats shared by probe files, CPU verification and GPU uploads.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace openrac::lighting {
// Stable on-disk identifiers; automatic selection is never written into an asset.
enum class ProbeTextureFormat : std::uint32_t {
    R11G11B10 = 0,
    Bc6h = 1,
    Astc8x8 = 2,
    Automatic = 3
};
// Returns the encoded byte count, rejecting invalid formats and overflowing dimensions.
std::size_t probe_texture_bytes(ProbeTextureFormat format, unsigned width, unsigned height);
// Returns the codec's square block extent, or one for uncompressed colour.
unsigned probe_texture_block(ProbeTextureFormat format);
// Reports a stable human-readable format name for bake and upload diagnostics.
const char* probe_texture_name(ProbeTextureFormat format);
// Parses the explicit codec override used by diagnostic and cross-device bake commands.
ProbeTextureFormat parse_probe_texture_format(std::string_view token);
// Returns a command-line token, distinct from the human-readable diagnostic label.
const char* probe_texture_token(ProbeTextureFormat format);
// Validates a GPU payload using bounded row scratch, without retaining an expanded image.
void validate_probe_texture(
    ProbeTextureFormat format, unsigned width, unsigned height, std::span<const std::uint8_t> bytes
);
// Expands HDR blocks for offline checks; runtime uploads retain the original bytes.
std::vector<std::array<float, 3>> decode_probe_texture(
    ProbeTextureFormat format, unsigned width, unsigned height, std::span<const std::uint8_t> bytes
);
}  // namespace openrac::lighting
