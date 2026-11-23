// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The 2D layer's game images (game/common/frontend/ui_images.h): 4-bit, 8-bit and 32-bit decodes,
// the 256-colour palette's stored order, the content hash and the cache's reuse.
#include <array>
#include <cstdint>
#include <vector>

#include "game/common/frontend/ui_images.h"
#include "renderer/textures/texture.h"
#include "tests/check.h"

namespace {

using openrac::frontend::decode_ui_image;
using openrac::frontend::UiImageCache;

// A palette whose entry i is the colour (i, i, i, 0x80), written in the order `stored(i)` gives.
template <std::size_t N, typename Order>
std::vector<std::uint8_t> grey_palette(Order stored) {
    std::vector<std::uint8_t> palette(N * 4);
    for (std::uint32_t i = 0; i < N; ++i) {
        const std::uint32_t at = stored(i);
        palette[at * 4 + 0] = static_cast<std::uint8_t>(i);
        palette[at * 4 + 1] = static_cast<std::uint8_t>(i);
        palette[at * 4 + 2] = static_cast<std::uint8_t>(i);
        palette[at * 4 + 3] = 0x80;
    }
    return palette;
}

// 4 bits a pixel: the low nibble is the first pixel.
void test_four_bits_low_nibble_first() {
    openrac_game_ui_image image{};
    image.width = 2;
    image.height = 1;
    image.bits = 4;
    image.palette = 1;
    const std::vector<std::uint8_t> pixels = {0x5A};
    const auto palette = grey_palette<16>([](std::uint32_t i) { return i; });
    const auto out = decode_ui_image(image, pixels, palette);
    CHECK(out.width == 2 && out.height == 1);
    CHECK(out.rgba.size() == 8);
    CHECK(out.rgba[0] == 0xA && out.rgba[4] == 0x5);
    CHECK(out.rgba[3] == 0x80);
}

// 8 bits a pixel through a palette in the game's stored order: entries 8-15 and 16-23 of each 32
// trade places.
void test_eight_bits_stored_palette() {
    openrac_game_ui_image image{};
    image.width = 4;
    image.height = 1;
    image.bits = 8;
    image.palette = 1;
    image.swizzled = 1;
    const std::vector<std::uint8_t> pixels = {3, 9, 17, 40};
    const auto palette = grey_palette<256>([](std::uint32_t i) {
        return ((i >> 3) & 1) != ((i >> 4) & 1) ? i ^ 0x18 : i;
    });
    const auto out = decode_ui_image(image, pixels, palette);
    CHECK(out.rgba.size() == 16);
    CHECK(out.rgba[0] == 3 && out.rgba[4] == 9 && out.rgba[8] == 17 && out.rgba[12] == 40);
}

// 32 bits a pixel: copied as they are; a short buffer decodes to nothing.
void test_thirty_two_bits_and_short_input() {
    openrac_game_ui_image image{};
    image.width = 1;
    image.height = 2;
    image.bits = 32;
    const std::vector<std::uint8_t> pixels = {1, 2, 3, 4, 5, 6, 7, 8};
    const auto out = decode_ui_image(image, pixels, {});
    CHECK(out.rgba == pixels);
    const auto cut = decode_ui_image(image, std::span(pixels).first(4), {});
    CHECK(cut.rgba.empty());
}

// The cache reads game memory by address, decodes once per content and follows changes from one
// tick to the next.
void test_cache_by_content() {
    std::vector<std::uint8_t> ram(0x1000);
    openrac_game_ui_image image{};
    image.pixels = 0x100;
    image.palette = 0x200;
    image.width = 8;
    image.height = 8;
    image.bits = 8;
    for (int i = 0; i < 256; ++i) {
        ram[0x200 + i * 4] = static_cast<std::uint8_t>(i);
        ram[0x200 + i * 4 + 3] = 0x80;
    }
    ram[0x100] = 7;
    UiImageCache cache;
    const auto first = cache.find(ram, image);
    CHECK(first.image != nullptr);
    CHECK(first.image != nullptr && first.image->pixels[0] == 7);
    const auto again = cache.find(ram, image);
    CHECK(again.image == first.image && again.hash == first.hash);
    ram[0x120] = 9;  // between the first and last 16 bytes: a tick reads it once
    const auto same_tick = cache.find(ram, image);
    CHECK(same_tick.hash == first.hash);
    ram[0x100] = 8;  // its first bytes: read again, even in the same tick
    const auto rewritten = cache.find(ram, image);
    CHECK(rewritten.hash != first.hash);
    cache.next_tick();
    const auto changed = cache.find(ram, image);
    CHECK(changed.hash != first.hash);
    CHECK(changed.image != nullptr && changed.image->pixels[0] == 8);
    image.pixels = 0x2000;  // past the memory
    CHECK(cache.find(ram, image).image == nullptr);
}

}  // namespace

int main() {
    test_four_bits_low_nibble_first();
    test_eight_bits_stored_palette();
    test_thirty_two_bits_and_short_input();
    test_cache_by_content();
    return openrac::test::result();
}
