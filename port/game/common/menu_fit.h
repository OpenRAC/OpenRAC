// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// How wide a mod's page of the game's widgets is drawn (mod_ui.cpp): the game's string width and
// the stretch of a page's frames that fits its texts. Header only, so the choice is tested without
// the game (tests/menu_fit_test.cpp).
//
// In Ratchet & Clank a widget's box is the frame moby's projected frame (pause.c,
// render_level_effects_and_screen_sprites): the frame its seq poses, as wide as the Options and
// Language pages' short labels need. A page of a mod stretches its frames sideways by one factor,
// each about its own centre, so that its widest title, item and footer line fit as the game lays
// them out, and no frame leaves the screen's safe area.
#pragma once

#include <algorithm>
#include <cstdint>
#include <span>
#include <string_view>

namespace openrac::frontend::menu_fit {

// The game's string width (rac1 measure_text_width, func_001F65B0): the sum of each character's
// width, the signed byte 3 of its 4-byte entry in the font's glyph table (by character code), up
// to the NUL. A character past the table counts nothing.
inline int text_width(std::string_view text, std::span<const std::uint8_t> glyphs) {
    int width = 0;

    for (const char c : text) {
        const auto code = static_cast<std::size_t>(static_cast<unsigned char>(c));

        if (code == 0) {
            break;
        }

        if (code * 4 + 3 < glyphs.size()) {
            width += static_cast<std::int8_t>(glyphs[code * 4 + 3]);
        }
    }

    return width;
}

// What each widget's drawing leaves around its line (pause.c), so a line that wide fits:
// - the title, a label (render_configured_text_label): a line centred in a window from x 1 to
//   width - 4, which wraps a wider one and scrolls it;
// - the list (draw_menu_text_list, centred rows): a line centred in a window from x 0 to
//   width - 2, which wraps a wider one;
// - the footer (render_localized_ui_entry_list): its own test, the small font once its widest
//   line is not 6 narrower than the frame.
constexpr int kTitlePadding = 8;
constexpr int kListPadding = 4;
constexpr int kFooterPadding = 6;
// A frame's width is a projection rounded to pixels: this much more, so a line that just fits at
// one stretch still does at the next tick's.
constexpr int kRounding = 2;

// The width lines of these widths need with `padding` around them; 0 for none.
inline float needed_width(std::span<const int> widths, int padding) {
    int widest = 0;

    for (const int w : widths) {
        widest = std::max(widest, w);
    }

    return widest > 0 ? static_cast<float>(widest + padding + kRounding) : 0.0f;
}

// One of a page's frames: its width as its seq poses it (stretch 1; 0 while not known), its centre
// on the screen, and the width its text needs (0 for none).
struct Frame {
    float width = 0.0f;
    float centre = 0.0f;
    float needed = 0.0f;
};

// The stretch for all of a page's frames: the most any frame needs (its text's width over its
// own), at least 1; at most what keeps every frame, widened about its centre, between `left` and
// `right` on the screen. Frames whose width is not known yet count for neither.
inline float stretch(std::span<const Frame> frames, float left, float right) {
    float wanted = 1.0f;
    float most = -1.0f;

    for (const Frame& f : frames) {
        if (!(f.width > 0.0f)) {
            continue;
        }

        wanted = std::max(wanted, f.needed / f.width);
        const float room = 2.0f * std::min(f.centre - left, right - f.centre) / f.width;
        most = most < 0.0f ? room : std::min(most, room);
    }

    if (most >= 0.0f) {
        wanted = std::min(wanted, most);
    }

    return std::max(wanted, 1.0f);
}

}  // namespace openrac::frontend::menu_fit
