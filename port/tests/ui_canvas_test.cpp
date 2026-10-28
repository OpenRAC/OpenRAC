// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The CPU picture the developer menu and the mods' menu pages draw on (renderer/ui/canvas.h):
// rectangles clipped to it and blended by their alpha, and text in its 5 by 7 font at a size, which
// lights the glyph's pixels and nothing else. The expected pixels are the font's own rows.

#include "check.h"
#include "renderer/ui/canvas.h"

namespace {

using openrac::renderer::ui::Canvas;
using openrac::renderer::ui::kGlyphCellHeight;
using openrac::renderer::ui::kGlyphCellWidth;
using openrac::renderer::ui::rgba;

constexpr std::uint32_t kBlack = rgba(0, 0, 0);
constexpr std::uint32_t kWhite = rgba(255, 255, 255);

void fills_inside_the_picture_only() {
    Canvas c(8, 4, kBlack);
    CHECK(c.pixels().size() == 8u * 4u * 4u);
    c.fill(6, 2, 10, 10, kWhite);  // runs off the right and the bottom
    CHECK(c.pixel(6, 2) == kWhite && c.pixel(7, 3) == kWhite);
    CHECK(c.pixel(5, 2) == kBlack && c.pixel(6, 1) == kBlack);
    c.fill(-3, -3, 4, 4, kWhite);  // runs off the left and the top
    CHECK(c.pixel(0, 0) == kWhite && c.pixel(1, 1) == kBlack);
    CHECK(c.pixel(8, 0) == 0 && c.pixel(-1, 0) == 0);  // outside there is no pixel
}

void blends_by_alpha() {
    Canvas c(2, 1, kBlack);
    c.fill(0, 0, 1, 1, rgba(255, 255, 255, 0));  // no alpha: nothing
    CHECK(c.pixel(0, 0) == kBlack);
    c.fill(0, 0, 2, 1, rgba(255, 0, 100, 128));
    CHECK(c.pixel(0, 0) == rgba(128, 0, 50, 255));
    c.clear(rgba(1, 2, 3, 4));
    CHECK(c.pixel(1, 0) == rgba(1, 2, 3, 4));
}

// The lit pixels of "I" at size 1: " ### ", then "  #  " five times, then " ### ".
void draws_the_font() {
    Canvas c(20, 10, kBlack);
    const int after = c.draw_text(0, 0, "Ii", kWhite);
    CHECK(after == 2 * kGlyphCellWidth);
    CHECK(c.pixel(0, 0) == kBlack && c.pixel(1, 0) == kWhite && c.pixel(3, 0) == kWhite);
    CHECK(c.pixel(2, 3) == kWhite && c.pixel(1, 3) == kBlack);
    // The lower-case letter is drawn as its capital.
    CHECK(c.pixel(kGlyphCellWidth + 2, 3) == kWhite && c.pixel(kGlyphCellWidth + 1, 6) == kWhite);
    // The spacing column stays clear.
    CHECK(c.pixel(5, 0) == kBlack);
}

void scales_and_breaks_lines() {
    Canvas c(40, 40, kBlack);
    c.draw_text(0, 0, "_\n_", kWhite, 2);
    // "_" lights its last row: at size 2, rows 12 and 13 of each line.
    CHECK(c.pixel(0, 12) == kWhite && c.pixel(9, 13) == kWhite && c.pixel(0, 11) == kBlack);
    CHECK(c.pixel(0, kGlyphCellHeight * 2 + 12) == kWhite);
    // A character the font lacks is a blank cell.
    Canvas d(12, 8, kBlack);
    CHECK(d.draw_text(0, 0, "~~", kWhite) == 2 * kGlyphCellWidth);

    for (int y = 0; y < d.height(); ++y) {
        for (int x = 0; x < d.width(); ++x) {
            CHECK(d.pixel(x, y) == kBlack);
        }
    }
}

}  // namespace

int main() {
    fills_inside_the_picture_only();
    blends_by_alpha();
    draws_the_font();
    scales_and_breaks_lines();
    return openrac::test::result();
}
