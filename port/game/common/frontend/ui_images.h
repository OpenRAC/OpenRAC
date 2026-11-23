// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The 2D layer's images as the game keeps them in its memory (openrac/ui_item.h: a font page, a
// HUD picture), decoded to RGBA8 once per content. A data-format conversion only: 4 or 8-bit
// indices through a 16 or 256-colour palette (the 256-colour ones in the game's stored order), or
// 32-bit colours; alpha stays the game's (0x80 opaque).
#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

#include "openrac/ui_item.h"
#include "renderer/frame/ui_records.h"

namespace openrac::frontend {

// An image decoded to RGBA8, rows top first.
struct UiPixels {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> rgba;  // 4 bytes a texel, alpha in the game's units
};

// The pixels' and palette's byte sizes `image` names (0 when it names none it can hold).
std::size_t ui_image_pixel_bytes(const openrac_game_ui_image& image);
std::size_t ui_image_palette_bytes(const openrac_game_ui_image& image);

// Decodes `pixels` (ui_image_pixel_bytes) with `palette` (ui_image_palette_bytes; empty for 32
// bits) as `image` describes them. Empty when the description is not one this reads.
UiPixels decode_ui_image(
    const openrac_game_ui_image& image,
    std::span<const std::uint8_t> pixels,
    std::span<const std::uint8_t> palette
);

// What an image's content hashes to: its description's shape, its pixels and its palette.
std::uint64_t ui_image_hash(
    const openrac_game_ui_image& image,
    std::span<const std::uint8_t> pixels,
    std::span<const std::uint8_t> palette
);

// The images a game's 2D layer draws, kept decoded by content: a tick asks by description (the
// game's addresses), hashed once a tick; the decoded pixels are shared with the frames that draw
// them and kept while drawn.
class UiImageCache {
public:
    // The image `image` names in `ram` (game memory, its addresses as offsets): decoded the first
    // time its content is seen. No image when it names memory past `ram` or an unreadable shape.
    renderer::UiImage find(std::span<const std::uint8_t> ram, const openrac_game_ui_image& image);

    // Whether decoded 8-bit images keep their indices and palette (renderer::UiImage::indexed), for
    // a PCSX2-format texture pack's names: only while such a pack is loaded.
    void set_keep_indexed(bool keep) { m_keep_indexed = keep; }

    // A new tick: the game may have changed what its addresses hold. Content not drawn for
    // `kKeepTicks` ticks is let go.
    void next_tick();

    static constexpr std::uint32_t kKeepTicks = 600;

private:
    struct Kept {
        std::shared_ptr<const renderer::Rgba8Image> image;
        std::shared_ptr<const renderer::UiIndexed> indexed;
        std::uint32_t last_tick = 0;
    };

    std::unordered_map<std::uint64_t, Kept> m_by_hash;
    std::unordered_map<std::uint64_t, std::uint64_t> m_this_tick;  // description key -> hash
    std::uint32_t m_tick = 0;
    bool m_keep_indexed = false;
};

}  // namespace openrac::frontend
