// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Sharper pictures of parts of the 2D layer's textures: a built-in that paints a texture of the
// game's (the key prompts, game/common/key_prompts.h), or a mod (openrac_api.sharp_cells), gives a
// picture of its cells several times as big, and the texture pool draws the texture from a picture
// that big, with those cells in it, so they stay sharp on a 4K screen. Only the picture on the GPU
// changes: the texture's texels, its hash, its distance field and the game's memory stay the
// game's.

#pragma once

#include <cstdint>
#include <vector>

namespace openrac::renderer::sharp_cells {

// A cell of a texture: where it is (texels), its colours as the game's texels make them (RGB, w x h
// x 3 bytes, to know the texture by), its sharper picture (RGB, scale times as wide and tall), and
// the texels the game had there before it was painted (RGBA, w x h x 4; empty when not known), so a
// texture pack finds its picture of the game's own texture.
struct Cell {
    int x = 0, y = 0, w = 0, h = 0;
    std::vector<std::uint8_t> rgb;
    std::vector<std::uint8_t> sharp;
    std::vector<std::uint8_t> game;
};

// A texture of `width` x `height` texels with sharper cells.
struct Page {
    int width = 0, height = 0;
    int scale = 1;
    std::vector<Cell> cells;
};

// An RGBA picture, 4 bytes a texel, top row first.
struct Picture {
    int width = 0, height = 0;
    std::vector<std::uint8_t> pixels;
};

// Who gave pages: the built-ins (kBuiltIn, the key prompts) or a mod (its handle). Each owner's
// pages are its own: setting them leaves the others'.
using Owner = const void*;
inline constexpr Owner kBuiltIn = nullptr;

// Replaces `owner`'s pages (none: it has none). Set them before the texels are changed, so the
// texture's first conversion finds them. Any thread.
void set(Owner owner, std::vector<Page> pages);

// Replaces the built-ins' pages: set(kBuiltIn, pages).
void set(std::vector<Page> pages);

// The texture `rgba` (width x height, as converted) drawn sharper: every texel scaled up smoothly,
// and the cells of every page that matches it put in (a page matches when each of its cells has
// its colours there). The first matching page sets the scale, and the others of that scale add
// their cells; the built-ins' pages come first, then the mods' in the order they first set pages,
// and an earlier page's cells are drawn over a later one's. False when no page matches. Any
// thread.
bool picture(int width, int height, const std::uint8_t* rgba, Picture& out);

// A painted cell found by unpainted(): where it is, and its sharper picture (RGB, `scale` times as
// wide and tall).
struct Found {
    int x = 0, y = 0, w = 0, h = 0;
    int scale = 1;
    std::vector<std::uint8_t> sharp;
};

// The texture `rgba` (width x height) as the game made it: `game` gets its texels with the cells of
// every matching page put back to the game's own (Cell::game), and `cells` those cells. False when
// no page with the game's texels matches. Any thread.
bool unpainted(
    int width,
    int height,
    const std::uint8_t* rgba,
    std::vector<std::uint8_t>& game,
    std::vector<Found>& cells
);

}  // namespace openrac::renderer::sharp_cells
