// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Key prompts: while the player uses the keyboard and mouse, the game's button icons (Cross,
// Circle, L1...) show the player's keys instead ("SPACE", "LMB"), and go back to the game's when
// the gamepad is used again. Ratchet & Clank and Going Commando print a button as a 24 x 16 cell of
// row 96 of their three fonts: 256 x 128 texels of 8 bits each, one palette of 256 colours for the
// three (32-bit, alpha 0x80 opaque, in the GS's CSM1 order). The game sends the fonts to the GS from
// game memory each frame, so a cell changed there shows the next frame. Painting changes only the
// cells; the game's bytes are kept and put back. The palette is not changed: the keys use its entries nearest their colours. The fonts are
// only pictures: the game's logic never reads them.

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace openrac::key_prompts {

inline constexpr int kFontsMax = 3;
inline constexpr std::uint32_t kFontWidth = 256;
inline constexpr std::uint32_t kFontBytes = 256u * 128u;
inline constexpr std::uint32_t kPaletteBytes = 1024;

// The cells, by their place along row 96.
enum Cell {
    Cross,
    Square,
    Triangle,
    Circle,
    L1,
    L2,
    R1,
    R2,
    kCells
};

// What each cell shows: a key's icon label (key_labels.h icon_label), empty for the game's own.
using Labels = std::array<std::string, kCells>;

// A level's fonts in game memory.
struct Fonts {
    std::array<std::uint8_t*, kFontsMax> font{};
    int count = 0;
    std::uint8_t* palette = nullptr;
};

// Where a game keeps its fonts: the effect bank's address (a word) and the effect texture table
// (16 bytes a texture: +8 the texels' offset, +0xA the palette's, in quadwords from the bank).
// Fonts 1 and 2 always, the third where it is loaded. False when they are not all in memory.
bool find_fonts(
    std::uint8_t* memory, std::uint32_t ram, std::uint32_t bank, std::uint32_t table, Fonts& out
);

// FNV-1a of the cells of each font.
std::uint32_t hash(const Fonts& f);

// Paints `labels` as keys over the cells: false (nothing changed) when the palette has no clear or
// no opaque entry.
bool paint(const Fonts& f, const Labels& labels);

// Paints and puts back, tick by tick.
class Painter {
public:
    // `labels` while the keyboard and mouse are in use, all empty while the gamepad is; `f` the
    // fonts now. Paints when the fonts are new (a level loaded them, or another mod painted them)
    // or the labels changed; puts the game's cells back when the labels are all empty.
    void update(const Fonts& f, const Labels& labels);

    // Forgets what it painted (a level that is gone): nothing is put back.
    void forget() { m_painted = false; }

    bool painted() const { return m_painted; }

private:
    void restore(const Fonts& f);

    bool m_painted = false;
    std::uint32_t m_hash = 0;  // the fonts' hash just after painting
    Labels m_labels{};
    int m_count = 0;
    std::array<std::vector<std::uint8_t>, kFontsMax> m_cells;  // the game's cells, row by row
};

}  // namespace openrac::key_prompts
