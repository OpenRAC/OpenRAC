// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Key prompts painted over the game's button icons (key_prompts.h).

#include "key_prompts.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "key_labels.h"

namespace openrac::key_prompts {
namespace {

constexpr std::uint32_t kRow = 96;
constexpr int kCellW = 24, kCellH = 16;
// Where each cell starts along the row (the "oe" glyph sits between the buttons and the sticks).
constexpr std::array<std::uint32_t, kCells> kCellU = {0, 24, 48, 72, 96, 120, 144, 168, 208, 232};
// The shoulder tabs' face inside a cell: x 4..22, y 3..13 (the tab's border around it).
constexpr int kFaceX0 = 4, kFaceX1 = 23, kFaceY0 = 3, kFaceY1 = 14;

// clang-format off
// A..Z then 0..9: a row's bits, the leftmost pixel the highest of the glyph's width.
constexpr std::uint8_t kBig[36][7] = {
    {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},  // A
    {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E},  // B
    {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E},  // C
    {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E},  // D
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F},  // E
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10},  // F
    {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F},  // G
    {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},  // H
    {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E},  // I
    {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C},  // J
    {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11},  // K
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F},  // L
    {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11},  // M
    {0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11},  // N
    {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},  // O
    {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10},  // P
    {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D},  // Q
    {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11},  // R
    {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E},  // S
    {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04},  // T
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},  // U
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04},  // V
    {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A},  // W
    {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11},  // X
    {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04},  // Y
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F},  // Z
    {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E},  // 0
    {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E},  // 1
    {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F},  // 2
    {0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E},  // 3
    {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02},  // 4
    {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E},  // 5
    {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E},  // 6
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08},  // 7
    {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E},  // 8
    {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C},  // 9
};
constexpr std::uint8_t kSmall[36][5] = {
    {0x02, 0x05, 0x07, 0x05, 0x05},  // A
    {0x06, 0x05, 0x06, 0x05, 0x06},  // B
    {0x03, 0x04, 0x04, 0x04, 0x03},  // C
    {0x06, 0x05, 0x05, 0x05, 0x06},  // D
    {0x07, 0x04, 0x06, 0x04, 0x07},  // E
    {0x07, 0x04, 0x06, 0x04, 0x04},  // F
    {0x03, 0x04, 0x05, 0x05, 0x03},  // G
    {0x05, 0x05, 0x07, 0x05, 0x05},  // H
    {0x07, 0x02, 0x02, 0x02, 0x07},  // I
    {0x01, 0x01, 0x01, 0x05, 0x02},  // J
    {0x05, 0x05, 0x06, 0x05, 0x05},  // K
    {0x04, 0x04, 0x04, 0x04, 0x07},  // L
    {0x05, 0x07, 0x07, 0x05, 0x05},  // M
    {0x06, 0x05, 0x05, 0x05, 0x05},  // N
    {0x02, 0x05, 0x05, 0x05, 0x02},  // O
    {0x06, 0x05, 0x06, 0x04, 0x04},  // P
    {0x02, 0x05, 0x05, 0x06, 0x03},  // Q
    {0x06, 0x05, 0x06, 0x05, 0x05},  // R
    {0x03, 0x04, 0x02, 0x01, 0x06},  // S
    {0x07, 0x02, 0x02, 0x02, 0x02},  // T
    {0x05, 0x05, 0x05, 0x05, 0x07},  // U
    {0x05, 0x05, 0x05, 0x05, 0x02},  // V
    {0x05, 0x05, 0x07, 0x07, 0x05},  // W
    {0x05, 0x05, 0x02, 0x05, 0x05},  // X
    {0x05, 0x05, 0x02, 0x02, 0x02},  // Y
    {0x07, 0x01, 0x02, 0x04, 0x07},  // Z
    {0x07, 0x05, 0x05, 0x05, 0x07},  // 0
    {0x02, 0x06, 0x02, 0x02, 0x07},  // 1
    {0x06, 0x01, 0x02, 0x04, 0x07},  // 2
    {0x06, 0x01, 0x02, 0x01, 0x06},  // 3
    {0x05, 0x05, 0x07, 0x01, 0x01},  // 4
    {0x07, 0x04, 0x06, 0x01, 0x06},  // 5
    {0x03, 0x04, 0x07, 0x05, 0x07},  // 6
    {0x07, 0x01, 0x02, 0x02, 0x02},  // 7
    {0x07, 0x05, 0x07, 0x05, 0x07},  // 8
    {0x07, 0x05, 0x07, 0x01, 0x06},  // 9
};
// clang-format on

// A palette index's place in CSM1 order: within each 32 entries the middle two groups of 8 trade.
std::uint32_t csm1(std::uint32_t i) {
    return (i & ~0x18u) | ((i & 8u) << 1) | ((i & 0x10u) >> 1);
}

std::uint8_t* row_of(std::uint8_t* font, std::uint32_t y) {
    return font + (kRow + y) * kFontWidth;
}

int glyph_index(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c - 'A';
    }
    if (c >= '0' && c <= '9') {
        return 26 + (c - '0');
    }
    return -1;
}

// A cell's picture: 0 the key's face, 1 white, 2 the letters' grey edge.
using Picture = std::array<std::array<std::uint8_t, kCellW>, kCellH>;

// The mouse, 9 x 10: its outline, its two buttons and the wheel between them.
constexpr std::array<std::uint16_t, 10> kMouse =
    {0x0FE, 0x111, 0x111, 0x111, 0x1FF, 0x101, 0x101, 0x101, 0x101, 0x0FE};
// The wheel's way, 5 x 3: up.
constexpr std::array<std::uint8_t, 3> kArrowUp = {0x04, 0x0E, 0x1F};

enum class MouseLit {
    None,
    Left,
    Right,
    Middle
};

void put(Picture& p, int x, int y, std::uint8_t v) {
    if (x >= kFaceX0 && x < kFaceX1 && y >= kFaceY0 && y < kFaceY1) {
        p[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)] = v;
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
    const int from = lit == MouseLit::Left ? 1 : lit == MouseLit::Right ? 5 : 3;
    if (lit != MouseLit::None) {
        for (int y = 1; y <= 3; ++y) {
            for (int x = from; x < from + 3; ++x) {
                put(p, left + x, top + y, 1);
            }
        }
    }
}

void draw_text(Picture& p, const std::string& text) {
    // Up to 3 letters in the large font, more in the small one; what does not fit is left out.
    const bool big = text.size() <= 3;
    const int w = big ? 5 : 3, h = big ? 7 : 5, advance = w + 1;
    const int n = std::min(static_cast<int>(text.size()), (kFaceX1 - kFaceX0 + 1) / advance);
    const int left = (kFaceX0 + kFaceX1 - (n * advance - 1)) / 2;
    const int top = (kFaceY0 + kFaceY1 - h) / 2;
    for (int i = 0; i < n; ++i) {
        const int g = glyph_index(text[static_cast<std::size_t>(i)]);
        if (g < 0) {
            continue;
        }
        for (int y = 0; y < h; ++y) {
            const std::uint8_t bits = big ? kBig[g][y] : kSmall[g][y];
            for (int x = 0; x < w; ++x) {
                if ((bits >> (w - 1 - x)) & 1u) {
                    put(p, left + i * advance + x, top + y, 1);
                }
            }
        }
    }
}

// What a cell shows for `label` (Labels).
Picture key_picture(const std::string& label) {
    Picture p{};
    const int top = (kFaceY0 + kFaceY1 - 10) / 2;
    if (label == "Mouse") {
        draw_mouse(p, 9, top, MouseLit::None);
    } else if (label == "MouseLeft" || label == "MouseRight" || label == "MouseMiddle") {
        const MouseLit lit = label == "MouseLeft"    ? MouseLit::Left
                             : label == "MouseRight" ? MouseLit::Right
                                                     : MouseLit::Middle;
        draw_mouse(p, 9, top, lit);
    } else if (label == "WheelUp" || label == "WheelDown") {
        draw_mouse(p, 6, top, MouseLit::Middle);
        for (int y = 0; y < 3; ++y) {
            const int row = label == "WheelUp" ? y : 2 - y;
            for (int x = 0; x < 5; ++x) {
                if ((kArrowUp[static_cast<std::size_t>(row)] >> (4 - x)) & 1u) {
                    put(p, 16 + x, top + 3 + y, 1);
                }
            }
        }
    } else {
        draw_text(p, openrac::keys::icon_label(label));
    }
    // The game's letters have a grey edge to their right.
    for (int y = kFaceY0; y < kFaceY1; ++y) {
        for (int x = kFaceX1 - 1; x > kFaceX0; --x) {
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
    for (int c = 0; c < kCells; ++c) {
        const std::string& label = labels[static_cast<std::size_t>(c)];
        if (label.empty()) {
            continue;
        }
        const Picture p = key_picture(label);
        for (int i = 0; i < f.count; ++i) {
            const Key& key = keys[static_cast<std::size_t>(i)];
            for (int y = 0; y < kCellH; ++y) {
                std::uint8_t* row =
                    row_of(f.font[static_cast<std::size_t>(i)], static_cast<std::uint32_t>(y))
                    + kCellU[static_cast<std::size_t>(c)];
                for (int x = 0; x < kCellW; ++x) {
                    const std::uint8_t v =
                        p[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
                    row[x] =
                        v == 1 ? key.white
                        : v == 2
                            ? key.edge
                            : key.cell[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
                }
            }
        }
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
