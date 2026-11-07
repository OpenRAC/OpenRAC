// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The launcher's colours in the game: the options drawer (debug/drawer.cpp) wears the theme the
// player picked in the launcher, and on Android the picked game's accent, as the launcher does. The
// launcher hands them over in OPENRAC_UI_COLORS (launcher/core/src/theme.rs), by app.css's names;
// a game started without it, or a name it does not hold, keeps the launcher's own (app.css's
// :root, the defaults below).

#pragma once

#include <cstdint>
#include <string_view>

namespace openrac::ui_colors {

// A colour as Dear ImGui's IM_COL32 packs it: red in the low byte, alpha in the high one.
using Color = std::uint32_t;

constexpr Color rgba(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255) {
    return static_cast<Color>(r) | static_cast<Color>(g) << 8 | static_cast<Color>(b) << 16
           | static_cast<Color>(a) << 24;
}

// app.css's tokens the menu (the drawer, the desktop's bar and window) draws with, under their
// names there. A game's accent (the launcher's palettes.json) sets the bar's and the accent's.
struct Colors {
    Color ink_deep = rgba(0x14, 0x12, 0x0e);    // --ink-deep: under everything; a switch's track
    Color panel = rgba(0x24, 0x22, 0x1e);       // --panel: the drawer
    Color panel_hi = rgba(0x2c, 0x2a, 0x26);    // --panel-hi: its cards
    Color brand = rgba(0xd6, 0xa0, 0x20);       // --brand: small labels
    Color bar_top = rgba(0xe5, 0xa0, 0x13);     // --bar-top, --bar-bottom: the bar's
    Color bar_bottom = rgba(0xcd, 0x83, 0x00);  //   gradient (the menu draws none now)
    Color bar_ink = rgba(0x1b, 0x12, 0x06);     // --bar-ink-deep: text on gold
    Color bar_label = rgba(0x2a, 0x18, 0x00);   // --bar-ink: dark text on the gold
    Color bar_stripe = rgba(0xc9, 0x92, 0x22);  // --bar-stripe, --bar-stripe-alt: the menu
    Color bar_stripe_alt = rgba(0xbd, 0x86, 0x19);  //   bar's two golds (debug/menu_window.cpp)
    Color amber = rgba(0xf5, 0xc0, 0x40);  // --amber: the focus, a main button, a switch that is on
    Color gold = rgba(0xff, 0xe0, 0x8f);   // --gold: values, headings
    Color text = rgba(0xe6, 0xe6, 0xeb);   // --text
    Color soft = rgba(0xb4, 0xb4, 0xbf);   // --soft: what is said under or beside
    Color dim = rgba(0x8b, 0x8b, 0x98);    // --dim: a switch's knob when off, small print
    Color line = rgba(255, 255, 255, 23);  // --line: hairlines between rows
    Color line_strong = rgba(255, 255, 255, 41);  // --line-strong
    Color err = rgba(0xff, 0x8a, 0x7a);           // --err: Quit
    // Control fills (launcher/docs/STYLE.md): no border; the menu's controls sit on a panel.
    Color control_card = rgba(0x2e, 0x2c, 0x28);     // --control-card
    Color control_card_hi = rgba(0x36, 0x34, 0x30);  // --control-card-hi: under the pointer
    // --glow: the light along the menu window's top, the game's colour (palettes.json accents).
    Color glow = rgba(0xf5, 0xc0, 0x40);
};

// `text` read over the defaults: NAME=#RRGGBB or NAME=#RRGGBBAA pairs, comma-separated, each NAME
// app.css's without its leading dashes ("panel-hi=#202020,line=#ffffff17").
Colors parse(std::string_view text);

// The colours this run was started with (OPENRAC_UI_COLORS), read once.
const Colors& launcher();

// `a` and `b` mixed, `t` (0..1) of the way to `b`, alpha too.
Color mix(Color a, Color b, float t);

// `c` with its alpha multiplied by `t` (0..1): a colour fading in or out.
Color fade(Color c, float t);

}  // namespace openrac::ui_colors
