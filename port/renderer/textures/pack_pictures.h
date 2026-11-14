// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// A texture pack's pictures that are not PNGs (docs/port/TEXTURE_PACKS.md), read into RGBA8 on
// the CPU so that a pack works the same on every platform and GPU:
// - KTX2 with Basis Universal (UASTC or ETC1S), or plain RGBA8 (Zstandard or none). Formats only
//   some GPUs read (BC1-BC7, ASTC, ETC2) are refused, so no pack works on one device only.
// - SVG, drawn at four times the size of the texture it replaces (nanosvg). It cannot reach outside its file: nanosvg
//   reads no links, images, fonts or scripts.
// Pack files are untrusted: sizes are capped, and a bad file gives nothing, with the reason in `why`.
#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <utility>

#include "renderer/textures/texture.h"

namespace openrac::renderer::pack_pictures {

// The largest picture side and texel count read (4096 x 4096 is 64 MB of RGBA8).
inline constexpr int kMaxSide = 8192;
inline constexpr std::uint64_t kMaxTexels = std::uint64_t{4096} * 4096;

// The largest files read: a KTX2 holds at most an uncompressed picture of kMaxTexels (with room
// for its mips), an SVG is text.
inline constexpr std::uintmax_t kMaxKtx2Bytes = std::uintmax_t{128} << 20;
inline constexpr std::uintmax_t kMaxSvgBytes = std::uintmax_t{4} << 20;

// The first mip level of a KTX2 file's bytes (the GPU makes the smaller ones, as for a PNG).
std::optional<Rgba8Image> decode_ktx2(std::span<const std::uint8_t> bytes, std::string& why);

// An SVG's text drawn into width x height texels, stretched to fill them.
std::optional<Rgba8Image> rasterize_svg(std::string text, int width, int height, std::string& why);

// The size an SVG replacing a texture of width x height is drawn at: four times its size, the
// longest side at most kMaxSvgSide, never below the texture's own size.
inline constexpr int kSvgScale = 4;
inline constexpr int kMaxSvgSide = 4096;
std::pair<int, int> svg_size(int width, int height);

// The files, read with the caps above; the log says why one fails. An SVG is drawn at svg_size()
// of the texture it replaces, `width` x `height`.
std::optional<Rgba8Image> load_ktx2(const std::filesystem::path& file);
std::optional<Rgba8Image> load_svg(const std::filesystem::path& file, int width, int height);

}  // namespace openrac::renderer::pack_pictures
