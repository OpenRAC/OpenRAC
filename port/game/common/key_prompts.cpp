// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Key prompts painted over the game's button icons (key_prompts.h).

#include "key_prompts.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string_view>

#include "key_labels.h"
#include "renderer/textures/sharp_cells.h"

namespace openrac::key_prompts {
namespace {

constexpr std::uint32_t kRow = 96;
constexpr int kCellW = 24, kCellH = 16;
// Where each cell starts along the row (the "oe" glyph sits between the buttons and the sticks).
constexpr std::array<std::uint32_t, kCells> kCellU = {0, 24, 48, 72, 96, 120, 144, 168, 208, 232};
// The shoulder tabs' face inside a cell: x 4..22, y 3..13 (the tab's border around it).
constexpr int kFaceX0 = 4, kFaceX1 = 23, kFaceY0 = 3, kFaceY1 = 14;
// A narrow key, where the game's icon is a round button or a stick: the tab with four columns of its
// middle taken out, so it ends (x 19) where the round icon does and the text after it keeps its gap.
constexpr int kNarrowCut = 10, kNarrowLess = 4, kNarrowX1 = kFaceX1 - kNarrowLess;

// clang-format off
// A glyph: its character and its rows' bits, the leftmost pixel the highest of the glyph's width.
struct Big {
    char c;
    std::uint8_t rows[7];
};
struct Small {
    char c;
    std::uint8_t rows[5];
};

// 5 x 7, for up to three letters.
constexpr Big kBig[] = {
    // Signs for keys whose names do not fit a narrow key: return, shift, backspace and space.
    {'\x01', {0x01, 0x01, 0x09, 0x1F, 0x08, 0x00, 0x00}},
    {'\x02', {0x04, 0x0A, 0x11, 0x1B, 0x0A, 0x0A, 0x0E}},
    {'\x03', {0x00, 0x04, 0x08, 0x1F, 0x08, 0x04, 0x00}},
    {'\x04', {0x00, 0x00, 0x00, 0x00, 0x11, 0x11, 0x1F}},
    {'A', {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'B', {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}},
    {'C', {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}},
    {'D', {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E}},
    {'E', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}},
    {'F', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}},
    {'G', {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F}},
    {'H', {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'I', {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}},
    {'J', {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C}},
    {'K', {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}},
    {'L', {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}},
    {'M', {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}},
    {'N', {0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11}},
    {'O', {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {'P', {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}},
    {'Q', {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}},
    {'R', {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}},
    {'S', {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}},
    {'T', {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}},
    {'U', {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {'V', {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}},
    {'W', {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A}},
    {'X', {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}},
    {'Y', {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}},
    {'Z', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}},
    {'0', {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}},
    {'1', {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}},
    {'2', {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}},
    {'3', {0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E}},
    {'4', {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}},
    {'5', {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E}},
    {'6', {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}},
    {'7', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}},
    {'8', {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}},
    {'9', {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C}},
    {'-', {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00}},
    {'=', {0x00, 0x00, 0x1F, 0x00, 0x1F, 0x00, 0x00}},
    {'[', {0x0E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0E}},
    {']', {0x0E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0E}},
    {'\\', {0x10, 0x10, 0x08, 0x04, 0x02, 0x01, 0x01}},
    {'/', {0x01, 0x01, 0x02, 0x04, 0x08, 0x10, 0x10}},
    {'#', {0x0A, 0x0A, 0x1F, 0x0A, 0x1F, 0x0A, 0x0A}},
    {';', {0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x04, 0x08}},
    {'\'', {0x0C, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00}},
    {'`', {0x08, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00}},
    {',', {0x00, 0x00, 0x00, 0x00, 0x0C, 0x04, 0x08}},
    {'.', {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C}},
    {'*', {0x00, 0x04, 0x15, 0x0E, 0x15, 0x04, 0x00}},
    {'+', {0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00}},
};
// 3 x 5, for four or five letters and two lines.
constexpr Small kSmall[] = {
    {'A', {0x02, 0x05, 0x07, 0x05, 0x05}},
    {'B', {0x06, 0x05, 0x06, 0x05, 0x06}},
    {'C', {0x03, 0x04, 0x04, 0x04, 0x03}},
    {'D', {0x06, 0x05, 0x05, 0x05, 0x06}},
    {'E', {0x07, 0x04, 0x06, 0x04, 0x07}},
    {'F', {0x07, 0x04, 0x06, 0x04, 0x04}},
    {'G', {0x03, 0x04, 0x05, 0x05, 0x03}},
    {'H', {0x05, 0x05, 0x07, 0x05, 0x05}},
    {'I', {0x07, 0x02, 0x02, 0x02, 0x07}},
    {'J', {0x01, 0x01, 0x01, 0x05, 0x02}},
    {'K', {0x05, 0x05, 0x06, 0x05, 0x05}},
    {'L', {0x04, 0x04, 0x04, 0x04, 0x07}},
    {'M', {0x05, 0x07, 0x07, 0x05, 0x05}},
    {'N', {0x06, 0x05, 0x05, 0x05, 0x05}},
    {'O', {0x02, 0x05, 0x05, 0x05, 0x02}},
    {'P', {0x06, 0x05, 0x06, 0x04, 0x04}},
    {'Q', {0x02, 0x05, 0x05, 0x06, 0x03}},
    {'R', {0x06, 0x05, 0x06, 0x05, 0x05}},
    {'S', {0x03, 0x04, 0x02, 0x01, 0x06}},
    {'T', {0x07, 0x02, 0x02, 0x02, 0x02}},
    {'U', {0x05, 0x05, 0x05, 0x05, 0x07}},
    {'V', {0x05, 0x05, 0x05, 0x05, 0x02}},
    {'W', {0x05, 0x05, 0x07, 0x07, 0x05}},
    {'X', {0x05, 0x05, 0x02, 0x05, 0x05}},
    {'Y', {0x05, 0x05, 0x02, 0x02, 0x02}},
    {'Z', {0x07, 0x01, 0x02, 0x04, 0x07}},
    {'0', {0x07, 0x05, 0x05, 0x05, 0x07}},
    {'1', {0x02, 0x06, 0x02, 0x02, 0x07}},
    {'2', {0x06, 0x01, 0x02, 0x04, 0x07}},
    {'3', {0x06, 0x01, 0x02, 0x01, 0x06}},
    {'4', {0x05, 0x05, 0x07, 0x01, 0x01}},
    {'5', {0x07, 0x04, 0x06, 0x01, 0x06}},
    {'6', {0x03, 0x04, 0x07, 0x05, 0x07}},
    {'7', {0x07, 0x01, 0x02, 0x02, 0x02}},
    {'8', {0x07, 0x05, 0x07, 0x05, 0x07}},
    {'9', {0x07, 0x05, 0x07, 0x01, 0x06}},
    {'-', {0x00, 0x00, 0x07, 0x00, 0x00}},
    {'=', {0x00, 0x07, 0x00, 0x07, 0x00}},
    {'[', {0x06, 0x04, 0x04, 0x04, 0x06}},
    {']', {0x03, 0x01, 0x01, 0x01, 0x03}},
    {'\\', {0x04, 0x04, 0x02, 0x01, 0x01}},
    {'/', {0x01, 0x01, 0x02, 0x04, 0x04}},
    {'#', {0x05, 0x07, 0x05, 0x07, 0x05}},
    {';', {0x00, 0x02, 0x00, 0x02, 0x04}},
    {'\'', {0x02, 0x02, 0x00, 0x00, 0x00}},
    {'`', {0x04, 0x02, 0x00, 0x00, 0x00}},
    {',', {0x00, 0x00, 0x00, 0x02, 0x04}},
    {'.', {0x00, 0x00, 0x00, 0x00, 0x02}},
    {'*', {0x00, 0x05, 0x02, 0x05, 0x00}},
    {'+', {0x00, 0x02, 0x07, 0x02, 0x00}},
};
// clang-format on

// A palette index's place in CSM1 order: within each 32 entries the middle two groups of 8 trade.
std::uint32_t csm1(std::uint32_t i) {
    return (i & ~0x18u) | ((i & 8u) << 1) | ((i & 0x10u) >> 1);
}

std::uint8_t* row_of(std::uint8_t* font, std::uint32_t y) {
    return font + (kRow + y) * kFontWidth;
}

// A cell's picture: 0 the key's face, 1 white, 2 the letters' grey edge; and the face's left and
// right end (x1 past it): a tab's, or a narrow key's.
struct Picture : std::array<std::array<std::uint8_t, kCellW>, kCellH> {
    int x0 = kFaceX0;
    int x1 = kFaceX1;
};

// The mouse, 9 x 10: its outline, its two buttons and the wheel between them.
constexpr std::array<std::uint16_t, 10> kMouse =
    {0x0FE, 0x111, 0x111, 0x111, 0x1FF, 0x101, 0x101, 0x101, 0x101, 0x0FE};
// A wheel's way, 5 x 3, pointing up.
constexpr std::uint8_t kWheelArrow[3] = {0x04, 0x0E, 0x1F};
// An arrow key's arrow, 7 x 7, pointing up.
constexpr std::uint8_t kArrow[7] = {0x08, 0x1C, 0x2A, 0x49, 0x08, 0x08, 0x08};

// The mouse's part drawn lit.
enum class MouseLit {
    None,
    Left,
    Right,
    Middle,
    Side
};

// Which way a picture drawn pointing up is turned.
enum class Turn {
    Up,
    Right,
    Down,
    Left
};

void put(Picture& p, int x, int y, std::uint8_t v) {
    if (x >= p.x0 && x < p.x1 && y >= kFaceY0 && y < kFaceY1) {
        p[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)] = v;
    }
}

// A picture of `w` x `h` (rows of bits, the leftmost pixel the highest), turned, its top left at
// `left`, `top`.
void draw_bits(Picture& p, int left, int top, const std::uint8_t* rows, int w, int h, Turn turn) {
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (((rows[y] >> (w - 1 - x)) & 1u) == 0) {
                continue;
            }
            int px = x, py = y;
            switch (turn) {
                case Turn::Up:
                    break;
                case Turn::Down:
                    py = h - 1 - y;
                    break;
                case Turn::Right:
                    px = h - 1 - y;
                    py = x;
                    break;
                case Turn::Left:
                    px = y;
                    py = w - 1 - x;
                    break;
            }
            put(p, left + px, top + py, 1);
        }
    }
}

void draw_mouse(Picture& p, int left, int top, MouseLit lit) {
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 9; ++x) {
            if ((kMouse[static_cast<std::size_t>(y)] >> (8 - x)) & 1u) {
                put(p, left + x, top + y, 1);
            }
        }
    }
    // The side buttons sit under the thumb, low on the left.
    if (lit == MouseLit::Side) {
        for (int y = 5; y <= 6; ++y) {
            for (int x = 1; x <= 2; ++x) {
                put(p, left + x, top + y, 1);
            }
        }
        return;
    }
    const int from = lit == MouseLit::Left ? 1 : lit == MouseLit::Right ? 5 : 3;
    if (lit != MouseLit::None) {
        for (int y = 1; y <= 3; ++y) {
            for (int x = from; x < from + 3; ++x) {
                put(p, left + x, top + y, 1);
            }
        }
    }
}

const std::uint8_t* glyph(char c, bool big) {
    if (big) {
        for (const Big& g : kBig) {
            if (g.c == c) {
                return g.rows;
            }
        }
    } else {
        for (const Small& g : kSmall) {
            if (g.c == c) {
                return g.rows;
            }
        }
    }
    return nullptr;
}

// One line of text, centred across the face, its top at `top`.
void draw_line(Picture& p, std::string_view text, bool big, int top) {
    const int w = big ? 5 : 3, h = big ? 7 : 5, advance = w + 1;
    const int n = std::min(static_cast<int>(text.size()), (p.x1 - p.x0 + 1) / advance);
    const int left = (p.x0 + p.x1 - (n * advance - 1)) / 2;
    for (int i = 0; i < n; ++i) {
        if (const std::uint8_t* rows = glyph(text[static_cast<std::size_t>(i)], big)) {
            draw_bits(p, left + i * advance, top, rows, w, h, Turn::Up);
        }
    }
}

// Few letters in the large font, more in the small one, two lines ("NUM\n4") in the small one
// one above the other; what does not fit is left out.
void draw_text(Picture& p, std::string_view text) {
    const std::size_t split = text.find('\n');
    if (split != std::string_view::npos) {
        draw_line(p, text.substr(0, split), false, kFaceY0);
        draw_line(p, text.substr(split + 1), false, kFaceY1 - 5);
        return;
    }
    // The large letters when they fit the face (up to 3 on a tab, 2 on a narrow key).
    const bool big = static_cast<int>(text.size()) * 6 - 1 <= p.x1 - p.x0;
    draw_line(p, text, big, (kFaceY0 + kFaceY1 - (big ? 7 : 5)) / 2);
}

// What a cell shows for `label` (Labels), on a tab or a narrow key.
Picture key_picture(const std::string& label, bool narrow) {
    Picture p{};
    p.x1 = narrow ? kNarrowX1 : kFaceX1;
    const int top = (kFaceY0 + kFaceY1 - 10) / 2;
    // The mouse alone, or a button of it lit.
    const auto mouse_only = [&](MouseLit lit) {
        draw_mouse(p, (p.x0 + p.x1 - 9) / 2, top, lit);
    };
    // The mouse on the left, the way it goes on the right.
    const auto mouse_way = [&](MouseLit lit, Turn turn) {
        const int left = narrow ? p.x0 : 6;
        draw_mouse(p, left, top, lit);
        const bool across = turn == Turn::Left || turn == Turn::Right;
        draw_bits(p, left + (across ? 11 : 10), top + (across ? 2 : 3), kWheelArrow, 5, 3, turn);
    };
    if (label == "Mouse") {
        mouse_only(MouseLit::None);
    } else if (label == "MouseLeft") {
        mouse_only(MouseLit::Left);
    } else if (label == "MouseRight") {
        mouse_only(MouseLit::Right);
    } else if (label == "MouseMiddle") {
        mouse_only(MouseLit::Middle);
    } else if (label == "WheelUp") {
        mouse_way(MouseLit::Middle, Turn::Up);
    } else if (label == "WheelDown") {
        mouse_way(MouseLit::Middle, Turn::Down);
    } else if (label == "WheelLeft") {
        mouse_way(MouseLit::Middle, Turn::Left);
    } else if (label == "WheelRight") {
        mouse_way(MouseLit::Middle, Turn::Right);
    } else if (label == "MouseBack") {
        mouse_way(MouseLit::Side, Turn::Left);
    } else if (label == "MouseForward") {
        mouse_way(MouseLit::Side, Turn::Right);
    } else if (label.starts_with("Arrow")) {
        const Turn turn = label == "ArrowUp"     ? Turn::Up
                          : label == "ArrowDown" ? Turn::Down
                          : label == "ArrowLeft" ? Turn::Left
                                                 : Turn::Right;
        draw_bits(p, (p.x0 + p.x1 - 7) / 2, (kFaceY0 + kFaceY1 - 7) / 2, kArrow, 7, 7, turn);
    } else {
        draw_text(
            p, narrow ? openrac::keys::narrow_icon_label(label) : openrac::keys::icon_label(label)
        );
    }
    // The game's letters have a grey edge to their right.
    for (int y = kFaceY0; y < kFaceY1; ++y) {
        for (int x = p.x1 - 1; x > p.x0; --x) {
            auto& row = p[static_cast<std::size_t>(y)];
            if (row[static_cast<std::size_t>(x)] == 0
                && row[static_cast<std::size_t>(x - 1)] == 1) {
                row[static_cast<std::size_t>(x)] = 2;
            }
        }
    }
    return p;
}

// The colour of a palette entry, and its sum of red, green and blue.
struct Rgb {
    double r = 0.0, g = 0.0, b = 0.0;
};

Rgb rgb(const std::uint8_t* palette, std::uint8_t i) {
    const std::uint8_t* e = palette + csm1(i) * 4u;
    return {static_cast<double>(e[0]), static_cast<double>(e[1]), static_cast<double>(e[2])};
}

double distance(Rgb a, Rgb b) {
    return std::fabs(a.r - b.r) + std::fabs(a.g - b.g) + std::fabs(a.b - b.b);
}

// A font's blank key: its L1 tab with the letters taken out, and the entries its letters take.
struct Key {
    std::array<std::array<std::uint8_t, kCellW>, kCellH> cell{};
    std::uint8_t white = 0;
    std::uint8_t edge = 0;
    double plane[3][3] = {};  // the face's shading: per channel, c0 + c1 x + c2 y
};

// The shoulder tabs' shading is a plane in x and y: fitted to the texels the four tabs (L1, L2, R1,
// R2) agree on, then every face texel far from it takes the tabs' entry nearest the plane's colour.
bool blank_key(const std::uint8_t* font, const std::uint8_t* palette, Key& out) {
    const auto tab = [&](int t, int x, int y) {
        return font
            [(kRow + static_cast<std::uint32_t>(y)) * kFontWidth + kCellU[L1 + t]
             + static_cast<std::uint32_t>(x)];
    };
    // The normal equations of a least-squares plane c0 + c1 x + c2 y, one per colour channel.
    double m[3][3] = {};
    double v[3][3] = {};
    std::array<bool, 256> used{};
    int samples = 0;
    for (int y = kFaceY0; y < kFaceY1; ++y) {
        for (int x = kFaceX0; x < kFaceX1; ++x) {
            double lo = 1e9, hi = -1e9;
            for (int t = 0; t < 4; ++t) {
                const Rgb c = rgb(palette, tab(t, x, y));
                lo = std::min(lo, c.r + c.g + c.b);
                hi = std::max(hi, c.r + c.g + c.b);
            }
            // The letters are bright: a texel bright in all four is a stroke they share.
            if (hi - lo >= 15.0 || hi >= 330.0) {
                continue;
            }
            const std::uint8_t i = tab(0, x, y);
            const Rgb c = rgb(palette, i);
            const double a[3] = {1.0, static_cast<double>(x), static_cast<double>(y)};
            const double ch[3] = {c.r, c.g, c.b};
            for (int r = 0; r < 3; ++r) {
                for (int k = 0; k < 3; ++k) {
                    m[r][k] += a[r] * a[k];
                    v[k][r] += a[r] * ch[k];
                }
            }
            used[i] = true;
            ++samples;
        }
    }
    const double det = m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1])
                       - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0])
                       + m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
    if (samples < 40 || std::fabs(det) < 1e-6) {
        return false;
    }
    // Cramer's rule for each channel.
    double coef[3][3] = {};
    for (int k = 0; k < 3; ++k) {
        for (int col = 0; col < 3; ++col) {
            double a[3][3];
            for (int r = 0; r < 3; ++r) {
                for (int c = 0; c < 3; ++c) {
                    a[r][c] = c == col ? v[k][r] : m[r][c];
                }
            }
            coef[k][col] = (a[0][0] * (a[1][1] * a[2][2] - a[1][2] * a[2][1])
                            - a[0][1] * (a[1][0] * a[2][2] - a[1][2] * a[2][0])
                            + a[0][2] * (a[1][0] * a[2][1] - a[1][1] * a[2][0]))
                           / det;
        }
    }
    std::memcpy(out.plane, coef, sizeof coef);
    // The letters' white: the brightest entry of the L1 tab; their edge: the tab's entry nearest a
    // middle grey.
    double brightest = -1.0, edge_off = 1e9;
    for (int y = 0; y < kCellH; ++y) {
        for (int x = 0; x < kCellW; ++x) {
            const std::uint8_t i = tab(0, x, y);
            out.cell[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)] = i;
            const Rgb c = rgb(palette, i);
            if (c.r + c.g + c.b > brightest) {
                brightest = c.r + c.g + c.b;
                out.white = i;
            }
            if (distance(c, {128.0, 128.0, 128.0}) < edge_off) {
                edge_off = distance(c, {128.0, 128.0, 128.0});
                out.edge = i;
            }
        }
    }
    for (int y = kFaceY0; y < kFaceY1; ++y) {
        for (int x = kFaceX0; x < kFaceX1; ++x) {
            const Rgb want{
                coef[0][0] + coef[0][1] * x + coef[0][2] * y,
                coef[1][0] + coef[1][1] * x + coef[1][2] * y,
                coef[2][0] + coef[2][1] * x + coef[2][2] * y
            };
            auto& texel = out.cell[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
            if (distance(rgb(palette, texel), want) <= 24.0) {
                continue;
            }
            double best = 1e9;
            for (int i = 0; i < 256; ++i) {
                const double d = distance(rgb(palette, static_cast<std::uint8_t>(i)), want);
                if (used[static_cast<std::size_t>(i)] && d < best) {
                    best = d;
                    texel = static_cast<std::uint8_t>(i);
                }
            }
        }
    }
    return true;
}

// A font's narrow key for the cell whose own texels are `own` (a row of the font at the cell's left):
// the tab without kNarrowLess columns of its middle, its shading squeezed to match, then the cell's
// own texels past it (where the game's round icon leaves them empty).
Key narrow_key(const Key& wide, const std::uint8_t* own) {
    Key k = wide;
    for (int y = 0; y < kCellH; ++y) {
        auto& row = k.cell[static_cast<std::size_t>(y)];
        const auto& from = wide.cell[static_cast<std::size_t>(y)];
        for (int x = kNarrowCut; x < kCellW; ++x) {
            row[static_cast<std::size_t>(x)] =
                x + kNarrowLess < kCellW
                    ? from[static_cast<std::size_t>(x + kNarrowLess)]
                    : own[static_cast<std::size_t>(y) * kFontWidth + static_cast<std::size_t>(x)];
        }
    }
    // The same shading from the face's left end to its right one, over the shorter face.
    const double squeeze = static_cast<double>(kFaceX1 - kFaceX0) / (kNarrowX1 - kFaceX0);
    for (auto& channel : k.plane) {
        channel[0] += channel[1] * kFaceX0 * (1.0 - squeeze);
        channel[1] *= squeeze;
    }
    return k;
}

// How many times as big the sharp picture of a cell is: enough for a 4K screen (2160 lines over the
// game's 448).
constexpr int kSharp = 6;

// How much of a sharp pixel at `px`, `py` (in texels, a texel's centre at whole numbers) the white
// strokes of `white` cover: each white texel a dot one texel wide, joined to its white neighbours
// by strokes as wide (across a corner only where no side joins them), and a square of four white
// texels filled; smooth over one sharp pixel at the edge.
float stroke_cover(const std::array<std::array<bool, kCellW>, kCellH>& white, float px, float py) {
    const auto on = [&](int x, int y) {
        return x >= 0 && y >= 0 && x < kCellW && y < kCellH
               && white[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
    };
    // The distance from a point to the segment from (ax, ay) to (bx, by).
    const auto segment = [&](float ax, float ay, float bx, float by) {
        const float dx = bx - ax, dy = by - ay;
        const float t =
            std::clamp(((px - ax) * dx + (py - ay) * dy) / (dx * dx + dy * dy), 0.0f, 1.0f);
        return std::hypot(px - (ax + t * dx), py - (ay + t * dy));
    };
    float d = 2.0f;
    const int cx = static_cast<int>(std::floor(px)), cy = static_cast<int>(std::floor(py));
    for (int y = cy - 1; y <= cy + 1; ++y) {
        for (int x = cx - 1; x <= cx + 1; ++x) {
            if (!on(x, y)) {
                continue;
            }
            const float fx = static_cast<float>(x), fy = static_cast<float>(y);
            d = std::min(d, std::hypot(px - fx, py - fy));
            if (on(x + 1, y)) {
                d = std::min(d, segment(fx, fy, fx + 1.0f, fy));
            }
            if (on(x, y + 1)) {
                d = std::min(d, segment(fx, fy, fx, fy + 1.0f));
            }
            if (on(x + 1, y + 1) && !on(x + 1, y) && !on(x, y + 1)) {
                d = std::min(d, segment(fx, fy, fx + 1.0f, fy + 1.0f));
            }
            if (on(x - 1, y + 1) && !on(x - 1, y) && !on(x, y + 1)) {
                d = std::min(d, segment(fx, fy, fx - 1.0f, fy + 1.0f));
            }
            if (on(x + 1, y) && on(x, y + 1) && on(x + 1, y + 1) && px >= fx && px <= fx + 1.0f
                && py >= fy && py <= fy + 1.0f) {
                d = 0.0f;
            }
        }
    }
    return std::clamp((0.5f - d) * static_cast<float>(kSharp) + 0.5f, 0.0f, 1.0f);
}

// A cell's picture kSharp times as big (RGB): the key's face shaded smoothly, the tab around it
// scaled up from its texels, and the strokes and their grey edge drawn sharp.
std::vector<std::uint8_t> sharp_picture(
    const Picture& p, const Key& key, const std::uint8_t* palette
) {
    std::array<std::array<bool, kCellW>, kCellH> white{};
    std::array<std::array<Rgb, kCellW>, kCellH> under{};
    for (int y = 0; y < kCellH; ++y) {
        for (int x = 0; x < kCellW; ++x) {
            const auto sy = static_cast<std::size_t>(y), sx = static_cast<std::size_t>(x);
            white[sy][sx] = p[sy][sx] == 1;
            const bool face = x >= p.x0 && x < p.x1 && y >= kFaceY0 && y < kFaceY1;
            const auto plane = [&](int k) {
                return std::clamp(
                    key.plane[k][0] + key.plane[k][1] * x + key.plane[k][2] * y, 0.0, 255.0
                );
            };
            under[sy][sx] =
                face ? Rgb{plane(0), plane(1), plane(2)} : rgb(palette, key.cell[sy][sx]);
        }
    }
    const Rgb light = rgb(palette, key.white), edge = rgb(palette, key.edge);
    const int w = kCellW * kSharp, h = kCellH * kSharp;
    std::vector<std::uint8_t> out(static_cast<std::size_t>(w * h * 3));
    for (int y = 0; y < h; ++y) {
        const float py = (static_cast<float>(y) + 0.5f) / kSharp - 0.5f;
        for (int x = 0; x < w; ++x) {
            const float px = (static_cast<float>(x) + 0.5f) / kSharp - 0.5f;
            // The texels under the strokes, as the linear filter reads them.
            const int x0 = static_cast<int>(std::floor(px)), y0 = static_cast<int>(std::floor(py));
            const double fx = px - static_cast<float>(x0), fy = py - static_cast<float>(y0);
            const auto at = [&](int tx, int ty) {
                return under[static_cast<std::size_t>(std::clamp(ty, 0, kCellH - 1))]
                            [static_cast<std::size_t>(std::clamp(tx, 0, kCellW - 1))];
            };
            const Rgb a = at(x0, y0), b = at(x0 + 1, y0), c = at(x0, y0 + 1),
                      d = at(x0 + 1, y0 + 1);
            const auto mix = [](double u, double v, double t) {
                return u + (v - u) * t;
            };
            Rgb colour{
                mix(mix(a.r, b.r, fx), mix(c.r, d.r, fx), fy),
                mix(mix(a.g, b.g, fx), mix(c.g, d.g, fx), fy),
                mix(mix(a.b, b.b, fx), mix(c.b, d.b, fx), fy)
            };
            // The grey edge: the strokes one texel to the right, under the strokes themselves.
            const double strokes = stroke_cover(white, px, py);
            const double shade = std::min<double>(
                stroke_cover(white, px - 1.0f, py),
                px <= static_cast<float>(p.x1) - 0.5f ? 1.0 : 0.0
            );
            colour =
                {mix(colour.r, edge.r, shade),
                 mix(colour.g, edge.g, shade),
                 mix(colour.b, edge.b, shade)};
            colour =
                {mix(colour.r, light.r, strokes),
                 mix(colour.g, light.g, strokes),
                 mix(colour.b, light.b, strokes)};
            std::uint8_t* o = out.data() + static_cast<std::size_t>((y * w + x) * 3);
            o[0] = static_cast<std::uint8_t>(std::lround(std::clamp(colour.r, 0.0, 255.0)));
            o[1] = static_cast<std::uint8_t>(std::lround(std::clamp(colour.g, 0.0, 255.0)));
            o[2] = static_cast<std::uint8_t>(std::lround(std::clamp(colour.b, 0.0, 255.0)));
        }
    }
    return out;
}

}  // namespace

bool find_fonts(
    std::uint8_t* memory,
    std::uint32_t ram,
    std::uint32_t bank_word,
    std::uint32_t table,
    Fonts& out
) {
    out = Fonts{};
    if (memory == nullptr || bank_word + 4 > ram || table + 16u * (kFontsMax + 1) > ram) {
        return false;
    }
    std::uint32_t bank = 0;
    std::memcpy(&bank, memory + bank_word, 4);
    if (bank == 0 || bank >= ram) {
        return false;
    }
    std::uint32_t palette = 0;
    for (int f = 0; f < kFontsMax; ++f) {
        const std::uint32_t record = table + 16u * static_cast<std::uint32_t>(f + 1);
        std::uint16_t texels16 = 0, clut16 = 0;
        std::memcpy(&texels16, memory + record + 8u, 2);
        std::memcpy(&clut16, memory + record + 0xAu, 2);
        // The third font is not loaded on the title's menus: its record is zero there.
        if (texels16 == 0 && clut16 == 0 && f >= 2) {
            break;
        }
        const std::uint32_t texels = bank + texels16 * 16u, clut = bank + clut16 * 16u;
        if (texels + kFontBytes > ram || clut + kPaletteBytes > ram || (f > 0 && clut != palette)) {
            return false;
        }
        out.font[static_cast<std::size_t>(out.count++)] = memory + texels;
        palette = clut;
    }
    out.palette = memory + palette;
    return out.count > 0;
}

std::uint32_t hash(const Fonts& f) {
    std::uint32_t h = 2166136261u;
    for (int i = 0; i < f.count; ++i) {
        for (std::uint32_t y = 0; y < kCellH; ++y) {
            const std::uint8_t* row = row_of(f.font[static_cast<std::size_t>(i)], y);
            for (std::uint32_t u = 0; u < kFontWidth; ++u) {
                h = (h ^ row[u]) * 16777619u;
            }
        }
    }
    return h;
}

bool paint(const Fonts& f, const Labels& labels) {
    if (f.count < 1 || f.palette == nullptr) {
        return false;
    }
    std::array<Key, kFontsMax> keys{};
    for (int i = 0; i < f.count; ++i) {
        if (!blank_key(
                f.font[static_cast<std::size_t>(i)], f.palette, keys[static_cast<std::size_t>(i)]
            )) {
            return false;
        }
    }
    // Each font's new rows of cells, and the renderer's sharp pictures of them.
    std::array<std::vector<std::uint8_t>, kFontsMax> rows;
    std::vector<renderer::sharp_cells::Page> pages(static_cast<std::size_t>(f.count));
    for (int i = 0; i < f.count; ++i) {
        const std::uint8_t* from = row_of(f.font[static_cast<std::size_t>(i)], 0);
        rows[static_cast<std::size_t>(i)].assign(from, from + kFontWidth * kCellH);
        pages[static_cast<std::size_t>(i)] =
            {static_cast<int>(kFontWidth), static_cast<int>(kFontBytes / kFontWidth), kSharp, {}};
    }
    for (int c = 0; c < kCells; ++c) {
        const std::string& label = labels[static_cast<std::size_t>(c)];
        if (label.empty()) {
            continue;
        }
        // The game's round buttons and sticks are narrower than its shoulder tabs, and its text
        // follows them closer: a narrow key there.
        const bool narrow = c < L1 || c > R2;
        const Picture p = key_picture(label, narrow);
        const std::uint32_t u = kCellU[static_cast<std::size_t>(c)];
        for (int i = 0; i < f.count; ++i) {
            const Key key = narrow ? narrow_key(
                                         keys[static_cast<std::size_t>(i)],
                                         rows[static_cast<std::size_t>(i)].data() + u
                                     )
                                   : keys[static_cast<std::size_t>(i)];
            renderer::sharp_cells::Cell cell{
                static_cast<int>(u), static_cast<int>(kRow), kCellW, kCellH, {}, {}
            };
            cell.rgb.reserve(static_cast<std::size_t>(kCellW * kCellH * 3));
            cell.game.reserve(static_cast<std::size_t>(kCellW * kCellH * 4));
            for (int y = 0; y < kCellH; ++y) {
                std::uint8_t* row = rows[static_cast<std::size_t>(i)].data()
                                    + static_cast<std::size_t>(y) * kFontWidth + u;
                for (int x = 0; x < kCellW; ++x) {
                    // The game's own texel, as the GS shows it (a pack's font is found by these).
                    const std::uint8_t* own = f.palette + csm1(row[x]) * 4u;
                    cell.game.insert(cell.game.end(), own, own + 4);
                    const std::uint8_t v =
                        p[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
                    row[x] =
                        v == 1 ? key.white
                        : v == 2
                            ? key.edge
                            : key.cell[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
                    const std::uint8_t* e = f.palette + csm1(row[x]) * 4u;
                    cell.rgb.insert(cell.rgb.end(), e, e + 3);
                }
            }
            cell.sharp = sharp_picture(p, key, f.palette);
            pages[static_cast<std::size_t>(i)].cells.push_back(std::move(cell));
        }
    }
    // The renderer knows the painted fonts before they are in memory, so it never draws them soft.
    renderer::sharp_cells::set(std::move(pages));
    for (int i = 0; i < f.count; ++i) {
        std::memcpy(
            row_of(f.font[static_cast<std::size_t>(i)], 0),
            rows[static_cast<std::size_t>(i)].data(),
            kFontWidth * kCellH
        );
    }
    return true;
}

void Painter::restore(const Fonts& f) {
    for (int i = 0; i < std::min(f.count, m_count); ++i) {
        const std::vector<std::uint8_t>& kept = m_cells[static_cast<std::size_t>(i)];
        for (std::uint32_t y = 0; y < kCellH; ++y) {
            std::memcpy(
                row_of(f.font[static_cast<std::size_t>(i)], y),
                kept.data() + y * kFontWidth,
                kFontWidth
            );
        }
    }
    m_painted = false;
    renderer::sharp_cells::set({});
}

void Painter::update(const Fonts& f, const Labels& labels) {
    if (f.count < 1) {
        return;
    }
    const bool want =
        std::any_of(labels.begin(), labels.end(), [](const std::string& l) { return !l.empty(); });
    // The game (a level loaded) or another mod changed them since: they are not ours to put back.
    if (m_painted && (hash(f) != m_hash || f.count != m_count)) {
        m_painted = false;
    }
    if (!want) {
        if (m_painted) {
            restore(f);
        }
        return;
    }
    if (m_painted && labels == m_labels) {
        return;
    }
    if (m_painted) {
        restore(f);
    }
    m_count = f.count;
    for (int i = 0; i < f.count; ++i) {
        std::vector<std::uint8_t>& kept = m_cells[static_cast<std::size_t>(i)];
        kept.resize(static_cast<std::size_t>(kFontWidth) * kCellH);
        for (std::uint32_t y = 0; y < kCellH; ++y) {
            std::memcpy(
                kept.data() + y * kFontWidth,
                row_of(f.font[static_cast<std::size_t>(i)], y),
                kFontWidth
            );
        }
    }
    if (paint(f, labels)) {
        m_painted = true;
        m_hash = hash(f);
        m_labels = labels;
    }
}

}  // namespace openrac::key_prompts
