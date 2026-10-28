// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// How wide a mod's page of the game's widgets is drawn (game/common/menu_fit.h): the game's string
// width over a glyph table, the width a widget's lines need, and the one stretch of a page's frames,
// at least 1 and capped to the safe area.

#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

#include "check.h"
#include "game/common/menu_fit.h"

namespace {

namespace fit = openrac::frontend::menu_fit;

bool near(float a, float b) {
    return std::fabs(a - b) < 1.0e-4f;
}

// A glyph table as the game's: 4 bytes a character, the width in byte 3.
std::vector<std::uint8_t> glyphs() {
    std::vector<std::uint8_t> table(256 * 4, 0);
    const auto set = [&](unsigned char c, int width) {
        table[c * 4 + 3] = static_cast<std::uint8_t>(static_cast<std::int8_t>(width));
    };

    for (unsigned char c = 'a'; c <= 'z'; ++c) {
        set(c, 8);
    }

    for (unsigned char c = 'A'; c <= 'Z'; ++c) {
        set(c, 14);
    }

    for (unsigned char c = '0'; c <= '9'; ++c) {
        set(c, 9);
    }

    set(' ', 5);
    set('.', 4);
    set(':', 4);
    set(0x10, 16);  // a pad glyph (Cross)
    set(0x7F, -2);  // a negative width counts as the game counts it
    return table;
}

}  // namespace

int main() {
    const std::vector<std::uint8_t> table = glyphs();

    // The game's sum, stopping at the NUL; characters without a width count nothing.
    CHECK(fit::text_width("", table) == 0);
    CHECK(fit::text_width("Join", table) == 14 + 3 * 8);
    CHECK(fit::text_width("192.168.1.20:7770", table) == 13 * 9 + 3 * 4 + 4);
    CHECK(fit::text_width(std::string_view("Ab\0cd", 5), table) == 22);
    CHECK(fit::text_width("\x10 Join", table) == 16 + 5 + 38);
    CHECK(fit::text_width("a\x7F", table) == 6);
    CHECK(fit::text_width("ab", std::span<const std::uint8_t>(table.data(), 'b' * 4)) == 8);

    // A widget's need: its widest line, its padding, the rounding and the margin; none for no line.
    const std::array<int, 3> lines = {120, 300, 0};
    CHECK(near(
        fit::needed_width(lines, fit::kFooterPadding), 300.0f + 6 + fit::kRounding + fit::kMargin
    ));
    CHECK(near(fit::needed_width(std::span<const int>(), fit::kListPadding), 0.0f));

    constexpr float kLeft = 26.0f, kRight = 486.0f;

    // Frames that fit their texts are left as the game made them.
    const std::array<fit::Frame, 3> fits = {{{200, 256, 150}, {240, 256, 180}, {220, 256, 0}}};
    CHECK(near(fit::stretch(fits, kLeft, kRight), 1.0f));

    // The frame that needs most decides for all of the page: the footer's 330 in 220.
    const std::array<fit::Frame, 3> footer = {{{200, 256, 150}, {240, 256, 300}, {220, 256, 330}}};
    CHECK(near(fit::stretch(footer, kLeft, kRight), 1.5f));

    // Capped where the widest frame reaches the safe area: 460 wide about 256 for a 240 frame.
    const std::array<fit::Frame, 3> long_text = {{{200, 256, 0}, {240, 256, 700}, {220, 256, 0}}};
    CHECK(near(fit::stretch(long_text, kLeft, kRight), 460.0f / 240.0f));

    // An off-centre frame is capped by its nearer side: centre 200, 174 to the left edge.
    const std::array<fit::Frame, 1> off = {{{100, 200, 1000}}};
    CHECK(near(fit::stretch(off, kLeft, kRight), 2.0f * 174.0f / 100.0f));

    // A frame not measured yet counts for nothing; with none, the game's width.
    const std::array<fit::Frame, 2> unknown = {{{0, 256, 900}, {200, 256, 260}}};
    CHECK(near(fit::stretch(unknown, kLeft, kRight), 1.3f));
    const std::array<fit::Frame, 1> none = {{{0, 0, 900}}};
    CHECK(near(fit::stretch(none, kLeft, kRight), 1.0f));

    // A frame already wider than the safe area is never narrowed.
    const std::array<fit::Frame, 1> wide = {{{500, 256, 600}}};
    CHECK(near(fit::stretch(wide, kLeft, kRight), 1.0f));

    return openrac::test::result();
}
