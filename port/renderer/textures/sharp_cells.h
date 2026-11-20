// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Sharper pictures of parts of the 2D layer's textures: a built-in that paints a texture of the
// game's (the key prompts, game/common/key_prompts.h) gives a picture of its cells several times
// as big, and the texture pool draws the texture from a picture that big, with those cells in it,
// so they stay sharp on a 4K screen. Only the picture on the GPU changes: the texture's texels, its
// hash, its distance field and the game's memory stay the game's.

#pragma once

#include <cstdint>
#include <vector>

namespace openrac::renderer::sharp_cells {

// A cell of a texture: where it is (texels), its colours as the game's texels make them (RGB, w x h
// x 3 bytes, to know the texture by), and its sharper picture (RGB, scale times as wide and tall).
struct Cell {
    int x = 0, y = 0, w = 0, h = 0;
    std::vector<std::uint8_t> rgb;
    std::vector<std::uint8_t> sharp;
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

// Replaces the pages (none: textures are drawn as before). Set it before the texels are changed,
// so the texture's first conversion finds it. Any thread.
void set(std::vector<Page> pages);

// The texture `rgba` (width x height, as converted) drawn sharper: every texel scaled up smoothly,
// the cells of the first page that matches it put in. False when no page matches. Any thread.
bool picture(int width, int height, const std::uint8_t* rgba, Picture& out);

}  // namespace openrac::renderer::sharp_cells
