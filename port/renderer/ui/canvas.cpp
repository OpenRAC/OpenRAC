// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "canvas.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstring>

namespace openrac::renderer::ui {

namespace {

// The glyphs' size in pixels, and the bytes a pixel has.
constexpr int kGlyphColumns = 5;
constexpr int kGlyphRows = 7;
constexpr std::size_t kBytesPerPixel = 4;
constexpr int kOpaque = 255;

// One 5 by 7 glyph: 7 rows of 5 columns, '#' lit.
struct Glyph {
    char c;
    const char* rows;
};

// The developer menu's font, with the punctuation a name, an address or a hint needs.
constexpr Glyph kFont[] = {
    {'A', " ### #   ##   #######   ##   ##   #"}, {'B', "#### #   ##   ##### #   ##   ##### "},
    {'C', " ### #   ##    #    #    #   # ### "}, {'D', "#### #   ##   ##   ##   ##   ##### "},
    {'E', "######    #    #### #    #    #####"}, {'F', "######    #    #### #    #    #    "},
    {'G', " ### #   ##    # ####   ##   # ####"}, {'H', "#   ##   ##   #######   ##   ##   #"},
    {'I', " ###   #    #    #    #    #   ### "}, {'J', "  ###   #    #    #    # #  #  ##  "},
    {'K', "#   ##  # # #  ##   # #  #  # #   #"}, {'L', "#    #    #    #    #    #    #####"},
    {'M', "#   ### ### # ##   ##   ##   ##   #"}, {'N', "#   ###  ## # ##  ###   ##   ##   #"},
    {'O', " ### #   ##   ##   ##   ##   # ### "}, {'P', "#### #   ##   ##### #    #    #    "},
    {'Q', " ### #   ##   ##   ## # ##  #  ## #"}, {'R', "#### #   ##   ##### # #  #  # #   #"},
    {'S', " #####    #     ###     #    ##### "}, {'T', "#####  #    #    #    #    #    #  "},
    {'U', "#   ##   ##   ##   ##   ##   # ### "}, {'V', "#   ##   ##   ##   ##   # # #   #  "},
    {'W', "#   ##   ##   ## # ## # ## # # # # "}, {'X', "#   ##   # # #   #   # # #   ##   #"},
    {'Y', "#   ##   # # #   #    #    #    #  "}, {'Z', "#####    #   #   #   #   #    #####"},
    {'0', " ### #   ##  ### # ###  ##   # ### "}, {'1', "  #   ##    #    #    #    #   ### "},
    {'2', " ### #   #    #   #   #   #   #####"}, {'3', "#####   #   #     #     ##   # ### "},
    {'4', "   #   ##  # # #  # #####   #    # "}, {'5', "######    ####     #    ##   # ### "},
    {'6', "  ##  #   #    #### #   ##   # ### "}, {'7', "#####    #   #   #   #    #    #   "},
    {'8', " ### #   ##   # ### #   ##   # ### "}, {'9', " ### #   ##   # ####    #   #  ##  "},
    {'(', "   #   #   #    #    #     #     # "}, {')', " #     #     #    #    #   #   #   "},
    {'-', "                ###                "}, {':', "      ##   ##        ##   ##       "},
    {'.', "                          ##   ##  "}, {'\'', "  #    #   #                       "},
    {'/', "    #    #   #   #   #   #    #    "}, {'>', " #     #     #     #   #   #   #   "},
    {',', "                     ##    #   #   "}, {'+', "       #    #  #####  #    #       "},
    {'_', "                              #####"}, {'!', "  #    #    #    #    #         #  "},
    {'?', " ### #   #    #   #   #         #  "}, {'<', "   #   #   #   #     #     #     # "},
    {'*', "     # # # ### ##### ### # # #     "}, {'#', " # # ##### # #  # # ##### # #      "},
    {'=', "          #####     #####          "}, {'[', " ###  #    #    #    #    #    ### "},
    {']', " ###    #    #    #    #    #  ### "}, {'"', " # #  # #                          "},
};

// Returns the rows of the glyph for `c` (its capital for a letter), or nullptr for none.
const char* glyph(char c) {
    const char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

    for (const Glyph& g : kFont) {
        // A glyph whose rows were mistyped short is no glyph.
        if (g.c == upper
            && std::strlen(g.rows) >= static_cast<std::size_t>(kGlyphColumns * kGlyphRows)) {
            return g.rows;
        }
    }

    return nullptr;
}

// Returns the byte `channel` (0 red, 1 green, 2 blue, 3 alpha) of a colour 0xRRGGBBAA.
std::uint8_t channel(std::uint32_t colour, int channel) {
    constexpr int kBitsPerChannel = 8;
    constexpr int kLastChannel = 3;
    return static_cast<std::uint8_t>(colour >> (kBitsPerChannel * (kLastChannel - channel)));
}

}  // namespace

Canvas::Canvas(int width, int height, std::uint32_t colour)
    : m_width(std::max(0, width)),
      m_height(std::max(0, height)),
      m_rgba(
          static_cast<std::size_t>(m_width) * static_cast<std::size_t>(m_height) * kBytesPerPixel
      ) {
    clear(colour);
}

void Canvas::clear(std::uint32_t colour) {
    for (std::size_t at = 0; at < m_rgba.size(); at += kBytesPerPixel) {
        for (int c = 0; c < static_cast<int>(kBytesPerPixel); ++c) {
            m_rgba[at + static_cast<std::size_t>(c)] = channel(colour, c);
        }
    }
}

void Canvas::fill(int x, int y, int w, int h, std::uint32_t colour) {
    const int alpha = channel(colour, 3);

    // A colour with no alpha changes nothing.
    if (alpha == 0) {
        return;
    }

    const int x0 = std::max(0, x);
    const int y0 = std::max(0, y);
    const int x1 = std::min(m_width, x + std::max(0, w));
    const int y1 = std::min(m_height, y + std::max(0, h));

    for (int row = y0; row < y1; ++row) {
        for (int col = x0; col < x1; ++col) {
            const std::size_t at =
                (static_cast<std::size_t>(row) * static_cast<std::size_t>(m_width)
                 + static_cast<std::size_t>(col))
                * kBytesPerPixel;

            for (int c = 0; c < 3; ++c) {
                // Over: the colour by its alpha, what was there by the rest.
                const int under = m_rgba[at + static_cast<std::size_t>(c)];
                const int over = channel(colour, c);
                m_rgba[at + static_cast<std::size_t>(c)] = static_cast<std::uint8_t>(
                    (over * alpha + under * (kOpaque - alpha) + kOpaque / 2) / kOpaque
                );
            }

            const int under_alpha = m_rgba[at + 3];
            m_rgba[at + 3] = static_cast<std::uint8_t>(
                alpha + (under_alpha * (kOpaque - alpha) + kOpaque / 2) / kOpaque
            );
        }
    }
}

int Canvas::draw_text(int x, int y, std::string_view text, std::uint32_t colour, int size) {
    const int scale = std::clamp(size, 1, kMaxTextSize);
    int at_x = x;
    int at_y = y;

    for (const char c : text) {
        // A newline goes back to the first column, a line down.
        if (c == '\n') {
            at_x = x;
            at_y += kGlyphCellHeight * scale;
            continue;
        }

        // A character the font has is drawn pixel by lit pixel; any other leaves its cell blank.
        if (const char* rows = glyph(c)) {
            for (int r = 0; r < kGlyphRows; ++r) {
                for (int col = 0; col < kGlyphColumns; ++col) {
                    // Only the glyph's lit pixels are drawn.
                    if (rows[r * kGlyphColumns + col] == '#') {
                        fill(at_x + col * scale, at_y + r * scale, scale, scale, colour);
                    }
                }
            }
        }

        at_x += kGlyphCellWidth * scale;
    }

    return at_x;
}

std::uint32_t Canvas::pixel(int x, int y) const {
    // Outside the picture there is no pixel.
    if (x < 0 || y < 0 || x >= m_width || y >= m_height) {
        return 0;
    }

    const std::size_t at = (static_cast<std::size_t>(y) * static_cast<std::size_t>(m_width)
                            + static_cast<std::size_t>(x))
                           * kBytesPerPixel;
    return rgba(m_rgba[at], m_rgba[at + 1], m_rgba[at + 2], m_rgba[at + 3]);
}

}  // namespace openrac::renderer::ui
