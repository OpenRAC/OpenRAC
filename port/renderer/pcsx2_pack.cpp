// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "renderer/pcsx2_pack.h"

#include <array>
#include <bit>
#include <cctype>
#include <charconv>
#include <format>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

#define XXH_INLINE_ALL
#include <xxhash.h>

#include "common/log.h"

namespace openrac::renderer::pcsx2_pack {
namespace {

namespace fs = std::filesystem;
std::unordered_map<std::string, fs::path> files;
LoadPng read_png = nullptr;

struct Crop {
    int left, top, right, bottom;
    std::uint64_t hash;
    fs::path file;
};

std::unordered_map<std::string, std::vector<Crop>> crops;
std::unordered_set<std::uint32_t> crop_formats;

std::string lowercase(std::string value) {
    for (char& c : value) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return value;
}

std::string key(std::uint64_t texture, std::uint64_t palette, std::uint32_t bits) {
    return std::format("{:x}-{:x}-{:08x}", texture, palette, bits);
}

template <class Integer>
bool hex(std::string_view part, Integer& value) {
    const auto end = part.data() + part.size();
    const auto result = std::from_chars(part.data(), end, value, 16);
    return result.ec == std::errc{} && result.ptr == end;
}

std::uint32_t texture_bits(const gs::Tex0& tex0) {
    return tex0.psm | (tex0.tw << 6) | (tex0.th << 10);
}

std::optional<std::string> parse_key(std::string_view name, const fs::path& path) {
    const auto first = name.find('-');
    const auto last = name.rfind('-');
    if (first == std::string_view::npos || first == last || name.size() - last != 9) {
        return {};
    }
    std::uint64_t texture = 0, palette = 0;
    std::uint32_t bits = 0;
    const auto region = name.find("-r", first + 1);
    const auto palette_end = region == std::string_view::npos ? last : region;
    if (!hex(name.substr(0, first), texture)
        || !hex(name.substr(first + 1, palette_end - first - 1), palette)
        || !hex(name.substr(last + 1), bits)) {
        return {};
    }
    // Older packs recorded TCC in bit 14. Modern PCSX2 ignores it.
    bits &= ~0x4000u;
    if (region == std::string_view::npos) {
        return key(texture, palette, bits);
    }
    // Legacy names store min/max (exclusive), X in the low word and Y in the high word.
    std::uint64_t area = 0;
    if (region == last || !hex(name.substr(region + 2, last - region - 2), area) || !area) {
        return {};
    }
    const auto side = [area](int shift) {
        return static_cast<int>((area >> shift) & 0xffff);
    };
    Crop crop{side(0), side(32), side(16), side(48), texture, path};
    const auto tw = (bits >> 6) & 15u;
    const auto th = (bits >> 10) & 15u;
    if (tw > 10 || th > 10 || (bits & 63u) != gs::kPsmt8) {
        return {};
    }
    if (crop.left == 0 && crop.right == 0) {
        crop.right = 1 << tw;
    }
    if (crop.top == 0 && crop.bottom == 0) {
        crop.bottom = 1 << th;
    }
    if (crop.right > crop.left && crop.bottom > crop.top && crop.right <= kMaxTextureSide
        && crop.bottom <= kMaxTextureSide) {
        crops[key(0, palette, bits)].push_back(std::move(crop));
        crop_formats.insert(bits);
    }
    return {};
}

std::uint64_t texture_hash(std::span<const std::uint8_t> raster, const gs::Tex0& tex0) {
    const int width = tex0.width();
    const int height = tex0.height();
    // PSMT8 hashes 16x16 GS blocks in raster block order. Sub-block textures and
    // high-byte textures hash their expanded indices without unrelated VRAM bytes.
    if (tex0.psm != gs::kPsmt8 || width < 16 || height < 16) {
        return XXH3_64bits(raster.data(), raster.size());
    }
    static const auto order = [] {
        std::array<std::size_t, 256> result{};
        for (int y = 0; y < 16; ++y) {
            for (int x = 0; x < 16; ++x) {
                result[swizzle::nibble_address(gs::kPsmt8, x, y, 0, 2) / 2] =
                    static_cast<std::size_t>(y * 16 + x);
            }
        }
        return result;
    }();
    XXH3_state_t state;
    XXH3_64bits_reset(&state);
    std::array<std::uint8_t, 256> block{};
    for (int y = 0; y < height; y += 16) {
        for (int x = 0; x < width; x += 16) {
            for (std::size_t i = 0; i < block.size(); ++i) {
                const auto offset = order[i];
                block[i] = raster
                    [static_cast<std::size_t>(y + static_cast<int>(offset / 16))
                         * static_cast<std::size_t>(width)
                     + static_cast<std::size_t>(x) + offset % 16];
            }
            XXH3_64bits_update(&state, block.data(), block.size());
        }
    }
    return XXH3_64bits_digest(&state);
}

std::optional<Rgba8Image> load(const fs::path& path) {
    if (lowercase(path.extension().string()) == ".dds") {
        return load_dds(path);
    }
    return read_png ? read_png(path) : std::nullopt;
}

const std::vector<Crop>* crops_for(const gs::Tex0& tex0, std::span<const std::uint32_t> palette) {
    if (crops.empty() || tex0.psm != gs::kPsmt8 || palette.size() != 256) {
        return nullptr;
    }
    const auto found =
        crops.find(key(0, XXH3_64bits(palette.data(), palette.size_bytes()), texture_bits(tex0)));
    return found != crops.end() ? &found->second : nullptr;
}

}  // namespace

void configure(const fs::path& replacements, LoadPng load_png) {
    read_png = load_png;
    files.clear();
    crops.clear();
    crop_formats.clear();
    if (replacements.empty()) {
        return;
    }
    std::error_code error;
    for (fs::recursive_directory_iterator
             it(replacements, fs::directory_options::skip_permission_denied, error),
         end;
         it != end;
         it.increment(error)) {
        const auto& path = it->path();
        const auto ext = lowercase(path.extension().string());
        if ((ext != ".dds" && ext != ".png") || !it->is_regular_file(error)) {
            continue;
        }
        if (auto name = parse_key(path.stem().string(), path)) {
            auto [found, inserted] = files.try_emplace(*name, path);
            if (!inserted && path < found->second) {
                found->second = path;
            }
        }
    }
    std::size_t crop_count = 0;
    for (auto& [group, entries] : crops) {
        // Shared one-texel margins belong to the next cell, where that cell starts.
        std::ranges::sort(entries, {}, [](const Crop& crop) {
            return std::tie(crop.top, crop.left, crop.file);
        });
        crop_count += entries.size();
    }
    if (!files.empty() || crop_count) {
        log::info(
            "PCSX2 texture pack {}: {} indexed replacements, {} cropped",
            replacements.string(),
            files.size(),
            crop_count
        );
    }
}

bool enabled() {
    return !files.empty() || !crops.empty();
}

bool replace(
    Rgba8Image& image,
    const gs::Tex0& tex0,
    std::span<const std::uint8_t> raster,
    std::span<const std::uint32_t> palette,
    AlphaScale alpha
) {
    if (files.empty() || tex0.tw > 10 || tex0.th > 10
        || (tex0.psm != gs::kPsmt8 && tex0.psm != gs::kPsmt8h) || palette.size() != 256
        || raster.size() != static_cast<std::size_t>(tex0.width() * tex0.height())) {
        return false;
    }
    const auto bits = texture_bits(tex0);
    const auto found = files.find(
        key(texture_hash(raster, tex0), XXH3_64bits(palette.data(), palette.size_bytes()), bits)
    );
    if (found == files.end()) {
        return false;
    }
    auto replacement = load(found->second);
    if (replacement && !replacement->pixels.empty()) {
        if (alpha == AlphaScale::Full) {
            // PCSX2 replacements retain GS alpha (0x80 is opaque).
            for (std::size_t a = 3; a < replacement->pixels.size(); a += 4) {
                replacement->pixels[a] =
                    static_cast<std::uint8_t>(std::min(255, replacement->pixels[a] * 2));
            }
        }
        image = std::move(*replacement);
        return true;
    }
    return false;
}

bool may_have_crops(const gs::Tex0& tex0) {
    return !crop_formats.empty() && crop_formats.contains(texture_bits(tex0));
}

std::array<int, 2> crop_extent(const gs::Tex0& tex0, std::span<const std::uint32_t> palette) {
    const auto* entries = crops_for(tex0, palette);
    if (!entries) {
        return {};
    }
    std::array extent{tex0.width(), tex0.height()};
    for (const auto& crop : *entries) {
        extent[0] = std::max(extent[0], crop.right);
        extent[1] = std::max(extent[1], crop.bottom);
    }
    return extent;
}

bool replace_crops(
    Rgba8Image& image,
    const gs::Tex0& tex0,
    std::span<const std::uint8_t> raster,
    std::span<const std::uint32_t> palette,
    std::array<int, 2> extent
) {
    const auto* entries = crops_for(tex0, palette);
    const auto [width, height] = extent;
    if (!entries || width < tex0.width() || height < tex0.height()
        || raster.size() != static_cast<std::size_t>(width) * height || image.pixels.empty()) {
        return false;
    }
    Rgba8Image atlas;
    int scale_x = 0, scale_y = 0;
    for (const auto& crop : *entries) {
        if (crop.right > width || crop.bottom > height) {
            continue;
        }
        const int w = crop.right - crop.left;
        const int h = crop.bottom - crop.top;
        // PCSX2 hashes a region's expanded indices row by row, even for swizzled textures.
        XXH3_state_t hash;
        XXH3_64bits_reset(&hash);
        for (int y = crop.top; y < crop.bottom; ++y) {
            XXH3_64bits_update(&hash, raster.data() + y * width + crop.left, w);
        }
        if (XXH3_64bits_digest(&hash) != crop.hash) {
            continue;
        }
        const auto picture = load(crop.file);
        if (!picture || picture->pixels.empty()) {
            continue;
        }
        if (atlas.pixels.empty()) {
            scale_x = std::max({1, (picture->width + w - 1) / w, image.width / tex0.width()});
            scale_y = std::max({1, (picture->height + h - 1) / h, image.height / tex0.height()});
            if (scale_x > 8192 / width || scale_y > 8192 / height
                || std::size_t{16} * 1024 * 1024
                       < static_cast<std::size_t>(width * scale_x) * height * scale_y) {
                continue;
            }
            atlas = Rgba8Image(width * scale_x, height * scale_y);
            for (int y = 0; y < atlas.height; ++y) {
                for (int x = 0; x < atlas.width; ++x) {
                    const int sx = x / scale_x;
                    const int sy = y / scale_y;
                    const auto pixel =
                        sx < tex0.width() && sy < tex0.height()
                            ? image.texel(
                                  static_cast<int>(
                                      std::int64_t{x} * image.width / (tex0.width() * scale_x)
                                  ),
                                  static_cast<int>(
                                      std::int64_t{y} * image.height / (tex0.height() * scale_y)
                                  )
                              )
                            : palette[raster[sy * width + sx]];
                    atlas.set_texel(x, y, pixel);
                }
            }
        }
        for (int y = 0; y < h * scale_y; ++y) {
            for (int x = 0; x < w * scale_x; ++x) {
                atlas.set_texel(
                    crop.left * scale_x + x,
                    crop.top * scale_y + y,
                    picture->texel(
                        x * picture->width / (w * scale_x), y * picture->height / (h * scale_y)
                    )
                );
            }
        }
        log::verbose("PCSX2 cropped texture: {}", crop.file.filename().string());
    }
    if (atlas.pixels.empty()) {
        return false;
    }
    image = std::move(atlas);
    return true;
}

bool replace_indexed8(
    Rgba8Image& image,
    std::span<const std::uint8_t> indices,
    std::span<const std::uint32_t> palette,
    AlphaScale alpha
) {
    const auto width = static_cast<unsigned>(image.width);
    const auto height = static_cast<unsigned>(image.height);
    if (!enabled() || width > kMaxTextureSide || height > kMaxTextureSide
        || !std::has_single_bit(width) || !std::has_single_bit(height)) {
        return false;
    }
    gs::Tex0 tex0{};
    tex0.psm = gs::kPsmt8;
    tex0.tw = static_cast<std::uint32_t>(std::countr_zero(width));
    tex0.th = static_cast<std::uint32_t>(std::countr_zero(height));
    return replace(image, tex0, indices, palette, alpha);
}

}  // namespace openrac::renderer::pcsx2_pack
