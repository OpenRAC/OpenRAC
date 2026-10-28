// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// A picture drawn on the CPU: filled rectangles and text in a 5 by 7 font, in RGBA8 pixels, for
// PictureBlit to put over the frame. The developer level menu (game/common/debug_menu.cpp) and the
// mods' menu pages (game/common/mod_menu.cpp, interface 1.5's menu_rect and menu_text) draw on it.
// No GL here: it is plain memory, so it is tested without a window.

#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace openrac::renderer::ui {

// A character's cell at size 1, in pixels: the 5 by 7 glyph, a column and a row between.
constexpr int kGlyphCellWidth = 6;
constexpr int kGlyphCellHeight = 8;

// The largest text size draw_text takes; a larger one is drawn at it.
constexpr int kMaxTextSize = 8;

// Returns the colour 0xRRGGBBAA of r, g, b and a (0 to 255 each).
constexpr std::uint32_t rgba(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255) {
    return (std::uint32_t{r} << 24) | (std::uint32_t{g} << 16) | (std::uint32_t{b} << 8) | a;
}

// An RGBA8 picture, its first row the top, that rectangles and text are drawn on. Owns its pixels.
class Canvas {
public:
    // A picture of `width` by `height` pixels, all `colour` (0xRRGGBBAA).
    Canvas(int width, int height, std::uint32_t colour = rgba(0, 0, 0));

    // Return the picture's size in pixels, and its bytes: width * height * 4, R G B A.
    int width() const { return m_width; }

    int height() const { return m_height; }

    const std::vector<std::uint8_t>& pixels() const { return m_rgba; }

    // Makes every pixel `colour`.
    void clear(std::uint32_t colour);

    // Fills `w` by `h` pixels from (x, y), the top left, with `colour`, blended over what is there
    // by its alpha; the part outside the picture is left out.
    void fill(int x, int y, int w, int h, std::uint32_t colour);

    // Draws `text` from (x, y), its top left, each character kGlyphCellWidth by kGlyphCellHeight
    // times `size` (1 to kMaxTextSize), in `colour`. Letters are drawn as capitals, a character
    // the font lacks as a blank, and a newline starts a line below. Returns the x after the last
    // line's last character.
    int draw_text(int x, int y, std::string_view text, std::uint32_t colour, int size = 1);

    // Returns the pixel at (x, y) as 0xRRGGBBAA; 0 outside the picture.
    std::uint32_t pixel(int x, int y) const;

private:
    // The size in pixels and the bytes, R G B A per pixel, row by row from the top.
    int m_width;
    int m_height;
    std::vector<std::uint8_t> m_rgba;
};

}  // namespace openrac::renderer::ui
