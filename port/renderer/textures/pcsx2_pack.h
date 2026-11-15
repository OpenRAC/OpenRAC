// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#pragma once

#include <array>
#include <filesystem>
#include <optional>
#include <string_view>

#include "renderer/textures/texture.h"

namespace openrac::renderer::pcsx2_pack {

// Called at startup before any scene or upload can request a replacement.
using LoadPng = std::optional<Rgba8Image> (*)(const std::filesystem::path&);
void configure(const std::filesystem::path& replacements, LoadPng load_png);
bool enabled();

// The uncompressed GS raster and the unscaled palette in index order.
// The image is changed only after an exact match is decoded successfully.
bool replace(
    Rgba8Image& image,
    const gs::Tex0& tex0,
    std::span<const std::uint8_t> raster,
    std::span<const std::uint32_t> palette,
    AlphaScale alpha = AlphaScale::Full
);

bool replace_indexed8(
    Rgba8Image& image,
    std::span<const std::uint8_t> indices,
    std::span<const std::uint32_t> palette,
    AlphaScale alpha = AlphaScale::Full
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
    std::array<int, 2> extent
);

std::optional<Rgba8Image> load_dds(const std::filesystem::path& file);

}  // namespace openrac::renderer::pcsx2_pack
