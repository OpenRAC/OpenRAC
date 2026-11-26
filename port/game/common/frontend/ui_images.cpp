// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The 2D layer's images decoded from the game's memory (ui_images.h).
#include "game/common/frontend/ui_images.h"

#include <algorithm>
#include <cstring>

#include "game/common/frontend/game_image_sources.h"
#include "openrac/guest.h"
#include "renderer/textures/texture.h"

namespace openrac::frontend {
namespace {

// The game's stored order of a 256-colour palette to its colour order: within each 32 entries,
// entries 8 to 15 and 16 to 23 trade places (bits 3 and 4 of the index). Its own inverse.
constexpr std::uint32_t stored_entry(std::uint32_t index) {
    return ((index >> 3) & 1) != ((index >> 4) & 1) ? index ^ 0x18 : index;
}

// FNV-1a over `bytes`, continuing from `hash`.
std::uint64_t fnv(std::uint64_t hash, std::span<const std::uint8_t> bytes) {
    for (const std::uint8_t b : bytes) {
        hash = (hash ^ b) * 0x100000001B3ull;
    }
    return hash;
}

bool readable(const openrac_game_ui_image& image) {
    const bool bits = image.bits == 4 || image.bits == 8 || image.bits == 32;
    return bits && image.width > 0 && image.height > 0 && image.width <= 4096
           && image.height <= 4096 && (image.bits == 32 || image.palette != 0);
}

// The bytes at game address `address` in `ram`, or none when they are not all there.
std::span<const std::uint8_t> bytes_at(
    std::span<const std::uint8_t> ram, std::uint32_t address, std::size_t size
) {
    const std::uint32_t at =
        openrac_guest_ram_address_for_size(address, static_cast<std::uint32_t>(ram.size()));
    if (size == 0 || at >= ram.size() || ram.size() - at < size) {
        return {};
    }
    return ram.subspan(at, size);
}

}  // namespace

std::size_t ui_image_pixel_bytes(const openrac_game_ui_image& image) {
    if (!readable(image)) {
        return 0;
    }
    return (std::size_t{image.width} * image.height * image.bits + 7) / 8;
}

std::size_t ui_image_palette_bytes(const openrac_game_ui_image& image) {
    if (!readable(image) || image.bits == 32) {
        return 0;
    }
    return image.bits == 4 ? 16u * 4u : 256u * 4u;
}

UiPixels decode_ui_image(
    const openrac_game_ui_image& image,
    std::span<const std::uint8_t> pixels,
    std::span<const std::uint8_t> palette
) {
    UiPixels out;
    if (pixels.size() < ui_image_pixel_bytes(image) || pixels.empty()
        || palette.size() < ui_image_palette_bytes(image)) {
        return out;
    }
    out.width = image.width;
    out.height = image.height;
    const std::size_t count = std::size_t{image.width} * image.height;
    out.rgba.resize(count * 4);
    for (std::size_t i = 0; i < count; ++i) {
        std::uint8_t* texel = out.rgba.data() + i * 4;
        if (image.bits == 32) {
            std::memcpy(texel, pixels.data() + i * 4, 4);
            continue;
        }
        std::uint32_t index = 0;
        if (image.bits == 8) {
            index = pixels[i];
            if (image.swizzled != 0) {
                index = stored_entry(index);
            }
        } else {
            const std::uint8_t pair = pixels[i / 2];
            index = (i & 1) != 0 ? pair >> 4 : pair & 0xFu;
        }
        std::memcpy(texel, palette.data() + index * 4, 4);
    }
    return out;
}

std::uint64_t ui_image_hash(
    const openrac_game_ui_image& image,
    std::span<const std::uint8_t> pixels,
    std::span<const std::uint8_t> palette
) {
    const std::uint8_t shape[6] = {
        static_cast<std::uint8_t>(image.width),
        static_cast<std::uint8_t>(image.width >> 8),
        static_cast<std::uint8_t>(image.height),
        static_cast<std::uint8_t>(image.height >> 8),
        image.bits,
        image.swizzled,
    };
    std::uint64_t hash = fnv(0xCBF29CE484222325ull, shape);
    hash = fnv(hash, pixels);
    return fnv(hash, palette);
}

renderer::UiImage UiImageCache::find(
    std::span<const std::uint8_t> ram, const openrac_game_ui_image& image
) {
    const std::size_t pixel_bytes = ui_image_pixel_bytes(image);
    if (pixel_bytes == 0) {
        return {};
    }
    // What the game handed over at an address and may not keep there (openrac_game_image_keep:
    // an image it sends once, from a buffer it reuses), else what memory holds there now.
    const GameImageSources& sources = game_image_sources();
    const auto read = [&](std::uint32_t address, std::size_t size) {
        const auto kept = sources.find(address, size);
        return !kept.empty() ? kept : bytes_at(ram, address, size);
    };
    // A tick reads an address's content once: keyed by the description and the image's first and
    // last 16 bytes, so a page the game rewrites between two draws of one tick (one buffer several
    // pictures go through) is read again.
    const auto edges = read(image.pixels, pixel_bytes);
    if (edges.empty()) {
        return {};
    }
    const std::size_t sample = std::min<std::size_t>(16, edges.size());
    std::uint64_t key = (std::uint64_t{image.pixels} << 32 | image.palette)
                        ^ (std::uint64_t{image.width} << 48) ^ (std::uint64_t{image.height} << 16)
                        ^ (std::uint64_t{image.bits} << 8) ^ image.swizzled;
    key = fnv(key, edges.first(sample));
    key = fnv(key, edges.last(sample));
    std::uint64_t hash = 0;
    if (const auto seen = m_this_tick.find(key); seen != m_this_tick.end()) {
        hash = seen->second;
    } else {
        const auto pixels = read(image.pixels, pixel_bytes);
        const std::size_t palette_bytes = ui_image_palette_bytes(image);
        const auto palette = palette_bytes != 0 ? read(image.palette, palette_bytes)
                                                : std::span<const std::uint8_t>{};
        if (pixels.empty() || palette.size() != palette_bytes) {
            return {};
        }
        hash = ui_image_hash(image, pixels, palette);
        if (!m_by_hash.contains(hash)) {
            UiPixels decoded = decode_ui_image(image, pixels, palette);
            if (decoded.rgba.empty()) {
                return {};
            }
            auto rgba = std::make_shared<renderer::Rgba8Image>();
            rgba->width = decoded.width;
            rgba->height = decoded.height;
            rgba->pixels = std::move(decoded.rgba);
            // For a PCSX2-format pack's names: the indices and the palette in index order.
            std::shared_ptr<renderer::UiIndexed> indexed;
            if (m_keep_indexed && image.bits == 8 && palette.size() == 256u * 4u) {
                indexed = std::make_shared<renderer::UiIndexed>();
                indexed->indices.assign(pixels.begin(), pixels.end());
                indexed->palette.resize(256);
                for (std::uint32_t i = 0; i < 256; ++i) {
                    const std::uint32_t at = image.swizzled != 0 ? stored_entry(i) : i;
                    std::memcpy(&indexed->palette[i], palette.data() + at * 4, 4);
                }
            }
            m_by_hash.emplace(hash, Kept{std::move(rgba), std::move(indexed), m_tick});
        }
        m_this_tick.emplace(key, hash);
    }
    Kept& kept = m_by_hash.at(hash);
    kept.last_tick = m_tick;
    return {hash, kept.image, kept.indexed};
}

void UiImageCache::next_tick() {
    ++m_tick;
    m_this_tick.clear();
    std::erase_if(m_by_hash, [this](const auto& entry) {
        return m_tick - entry.second.last_tick > kKeepTicks;
    });
}

}  // namespace openrac::frontend
