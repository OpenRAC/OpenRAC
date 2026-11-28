// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string_view>

#include "renderer/textures/texture.h"

namespace openrac::renderer::pcsx2_pack {

// Called at startup before any scene or upload can request a replacement.
using LoadPng = std::optional<Rgba8Image> (*)(const std::filesystem::path&);
void configure(const std::filesystem::path& replacements, LoadPng load_png);
bool enabled();

// Before configure(). `in_background`: a picture a texture needs and the cache lacks is read on
// another thread (a caller that passes `pending` draws the original until then). `precache`:
// reads the whole pack in the background from the start, smallest files first, until the cache
// (larger then) is full.
void set_loading(bool in_background, bool precache);
// Changes each time a picture a caller waited for (`pending`) is ready.
std::uint64_t ready_version();

// The uncompressed GS raster and the unscaled palette in index order.
// The image is changed only after an exact match is decoded successfully.
// `pending`, when not null, lets the picture load in the background: it is set when the picture
// is not ready, and the caller asks again when ready_version() changes.
bool replace(
    Rgba8Image& image,
    const gs::Tex0& tex0,
    std::span<const std::uint8_t> raster,
    std::span<const std::uint32_t> palette,
    AlphaScale alpha = AlphaScale::Full,
    bool* pending = nullptr
);

// `high`: the name of the same indices sent as the top byte of 32-bit words (PSMT8H).
bool replace_indexed8(
    Rgba8Image& image,
    std::span<const std::uint8_t> indices,
    std::span<const std::uint32_t> palette,
    AlphaScale alpha = AlphaScale::Full,
    bool high = false,
    bool* pending = nullptr
);

// Old PCSX2 names carry a rectangle in the source texture. The pool reads any
// extra texels past TEX0's nominal extent before asking for a composed atlas.
bool may_have_crops(const gs::Tex0& tex0);
std::array<int, 2> crop_extent(const gs::Tex0& tex0, std::span<const std::uint32_t> palette);
bool replace_crops(
    Rgba8Image& image,
    const gs::Tex0& tex0,
    std::span<const std::uint8_t> raster,
    std::span<const std::uint32_t> palette,
    std::array<int, 2> extent,
    bool* pending = nullptr
);

std::optional<Rgba8Image> load_dds(const std::filesystem::path& file);

}  // namespace openrac::renderer::pcsx2_pack
