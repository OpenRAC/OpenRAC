// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Key prompts painted over the game's button icons (key_prompts.h).

#include "key_prompts.h"

#include <algorithm>
#include <cstring>

namespace openrac::key_prompts {
namespace {

constexpr std::uint32_t kRow = 96;
constexpr int kCellW = 24, kCellH = 16;
constexpr std::uint32_t kCellsWidth = 192;  // the eight cells, u 0..191

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

// The key's colours, as the game's own L1 icon: its edge, its face and its letters.
constexpr std::uint8_t kEdge[3] = {190, 190, 200};
constexpr std::uint8_t kFace[3] = {34, 34, 40};
constexpr std::uint8_t kInk[3] = {245, 245, 245};

// A palette index's place in CSM1 order: within each 32 entries the middle two groups of 8 trade.
std::uint32_t csm1(std::uint32_t i) {
    return (i & ~0x18u) | ((i & 8u) << 1) | ((i & 0x10u) >> 1);
}

std::uint8_t* texel(std::uint8_t* font, std::uint32_t u, std::uint32_t y) {
    return font + (kRow + y) * kFontWidth + u;
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

// One cell's picture, as 0 (clear), 1 (edge), 2 (face) and 3 (letters).
using Picture = std::array<std::array<std::uint8_t, kCellW>, kCellH>;

Picture key_picture(const std::string& label) {
    Picture p{};
    for (int y = 1; y < kCellH - 1; ++y) {
        for (int x = 0; x < kCellW; ++x) {
            const bool corner = (x == 0 || x == kCellW - 1) && (y == 1 || y == kCellH - 2);
            if (corner) {
                continue;
            }
            const bool edge = x == 0 || x == kCellW - 1 || y == 1 || y == kCellH - 2;
            p[y][x] = edge ? 1 : 2;
        }
    }
    // Up to 3 letters in the large font, more in the small one; what does not fit is left out.
    const bool big = label.size() <= 3;
    const int w = big ? 5 : 3, h = big ? 7 : 5, advance = w + 1;
    const int most = (kCellW - 4 + 1) / advance;
    const int n = std::min(static_cast<int>(label.size()), most);
    const int left = (kCellW - (n * advance - 1)) / 2, top = (kCellH - h) / 2;
    for (int i = 0; i < n; ++i) {
        const int g = glyph_index(label[static_cast<std::size_t>(i)]);
        if (g < 0) {
            continue;
        }
        for (int y = 0; y < h; ++y) {
            const std::uint8_t bits = big ? kBig[g][y] : kSmall[g][y];
            for (int x = 0; x < w; ++x) {
                if ((bits >> (w - 1 - x)) & 1u) {
                    p[top + y][left + i * advance + x] = 3;
                }
            }
        }
    }
    return p;
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
            const std::uint8_t* row = texel(f.font[static_cast<std::size_t>(i)], 0, y);
            for (std::uint32_t u = 0; u < kCellsWidth; ++u) {
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
    // The palette's entries nearest the key's colours, and one with no alpha: the palette is left
    // as it is, as the game's other textures may use it too.
    std::array<int, 4> index{-1, -1, -1, -1};
    std::array<int, 4> best{};
    best.fill(1 << 30);
    const std::array<const std::uint8_t*, 3> want{kEdge, kFace, kInk};
    for (std::uint32_t i = 0; i < 256; ++i) {
        const std::uint8_t* e = f.palette + csm1(i) * 4u;
        if (e[3] == 0) {
            if (index[0] < 0) {
                index[0] = static_cast<int>(i);
            }
            continue;
        }
        if (e[3] < 0x80) {
            continue;
        }
        for (std::size_t k = 0; k < want.size(); ++k) {
            int d = 0;
            for (int c = 0; c < 3; ++c) {
                const int delta = e[c] - want[k][c];
                d += delta * delta;
            }
            if (d < best[k + 1]) {
                best[k + 1] = d;
                index[k + 1] = static_cast<int>(i);
            }
        }
    }
    if (std::find(index.begin(), index.end(), -1) != index.end()) {
        return false;
    }
    for (int c = 0; c < kCells; ++c) {
        const std::string& label = labels[static_cast<std::size_t>(c)];
        if (label.empty()) {
            continue;
        }
        const Picture p = key_picture(label);
        for (int i = 0; i < f.count; ++i) {
            for (int y = 0; y < kCellH; ++y) {
                std::uint8_t* row = texel(
                    f.font[static_cast<std::size_t>(i)],
                    static_cast<std::uint32_t>(c * kCellW),
                    static_cast<std::uint32_t>(y)
                );
                for (int x = 0; x < kCellW; ++x) {
                    row[x] = static_cast<std::uint8_t>(
                        index[p[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)]]
                    );
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
                texel(f.font[static_cast<std::size_t>(i)], 0, y),
                kept.data() + y * kCellsWidth,
                kCellsWidth
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
        kept.resize(kCellsWidth * kCellH);
        for (std::uint32_t y = 0; y < kCellH; ++y) {
            std::memcpy(
                kept.data() + y * kCellsWidth,
                texel(f.font[static_cast<std::size_t>(i)], 0, y),
                kCellsWidth
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
