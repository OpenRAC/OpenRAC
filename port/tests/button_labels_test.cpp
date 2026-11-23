// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The pad's buttons as the player's device has them (game/common/button_labels.h, mod interface
// 1.31): the game's icons on a PlayStation pad, letters on the others, the bound keys on the
// keyboard and mouse, and the overlay's text with its button tokens.

#include <cstring>
#include <string>

#include "button_labels.h"
#include "check.h"
#include "openrac/openrac_mod.h"

namespace bl = openrac::button_labels;

namespace {

bl::KeyNames vendor_keys() {
    bl::KeyNames k;
    k[0] = "Space";   // Cross
    k[1] = "KeyF";    // Square
    k[3] = "Escape";  // Circle
    k[8] = "WASD";    // the left stick
    k[9] = "Mouse";   // the right stick
    return k;
}

void playstation() {
    const bl::Device d;  // no pad connected, or a PlayStation one
    int kind = 0;
    CHECK(bl::label(OPENRAC_PAD_SQUARE, d, {}, kind) == OPENRAC_UI_SQUARE);
    CHECK(kind == OPENRAC_BUTTONS_PLAYSTATION);
    CHECK(bl::label(OPENRAC_PAD_CROSS, d, {}, kind) == OPENRAC_UI_CROSS);
    CHECK(bl::label(OPENRAC_PAD_R2, d, {}, kind) == OPENRAC_UI_R2);
    CHECK(bl::label(OPENRAC_PAD_L3, d, {}, kind) == OPENRAC_UI_LEFT_STICK);
    CHECK(bl::label(OPENRAC_PAD_START, d, {}, kind) == "START");
    // The keys a mod painted are the keyboard's only.
    CHECK(bl::label(OPENRAC_PAD_SQUARE, d, vendor_keys(), kind) == OPENRAC_UI_SQUARE);
    // The face buttons stay the font's icons; the shoulders are named.
    CHECK(bl::overlay_text(OPENRAC_UI_SQUARE " Buy all", d, {}) == "\x13 Buy all");
    CHECK(bl::overlay_text(OPENRAC_UI_L1 "/" OPENRAC_UI_R1, d, {}) == "L1/R1");
}

void letters() {
    bl::Device d;
    d.pad = bl::Pad::Xbox;
    int kind = 0;
    CHECK(bl::label(OPENRAC_PAD_SQUARE, d, {}, kind) == "X" && kind == OPENRAC_BUTTONS_LETTERS);
    CHECK(bl::label(OPENRAC_PAD_CROSS, d, {}, kind) == "A");
    CHECK(bl::label(OPENRAC_PAD_TRIANGLE, d, {}, kind) == "Y");
    CHECK(bl::label(OPENRAC_PAD_CIRCLE, d, {}, kind) == "B");
    CHECK(bl::label(OPENRAC_PAD_L2, d, {}, kind) == "LT");
    CHECK(bl::overlay_text(OPENRAC_UI_SQUARE " Buy all", d, {}) == "X Buy all");
    // A Nintendo pad by place: the bottom button is its B.
    d.pad = bl::Pad::Nintendo;
    CHECK(bl::label(OPENRAC_PAD_CROSS, d, {}, kind) == "B");
    CHECK(bl::label(OPENRAC_PAD_SQUARE, d, {}, kind) == "Y");
}

void keys() {
    bl::Device d;
    d.keys = true;
    d.pad = bl::Pad::Xbox;  // the keyboard is in use, whatever pad is connected
    int kind = 0;
    CHECK(bl::label(OPENRAC_PAD_SQUARE, d, vendor_keys(), kind) == "F");
    CHECK(kind == OPENRAC_BUTTONS_KEYS);
    CHECK(bl::label(OPENRAC_PAD_CROSS, d, vendor_keys(), kind) == "SPACE");
    CHECK(bl::label(OPENRAC_PAD_L3, d, vendor_keys(), kind) == "WASD");
    CHECK(bl::label(OPENRAC_PAD_R3, d, vendor_keys(), kind) == "MOUSE");
    // No key on the icon (none presses it here, or the prompts are off): the game's own icon.
    CHECK(bl::label(OPENRAC_PAD_TRIANGLE, d, vendor_keys(), kind) == OPENRAC_UI_TRIANGLE);
    CHECK(kind == OPENRAC_BUTTONS_PLAYSTATION);
    CHECK(bl::overlay_text(OPENRAC_UI_SQUARE " Buy all", d, vendor_keys()) == "[F] Buy all");
}

void not_a_button() {
    const bl::Device d;
    int kind = 0;
    CHECK(bl::label(0, d, {}, kind).empty() && kind == OPENRAC_HOOK_UNKNOWN);
    CHECK(bl::label(OPENRAC_PAD_CROSS | OPENRAC_PAD_SQUARE, d, {}, kind).empty());
    CHECK(kind == OPENRAC_HOOK_UNKNOWN);
    CHECK(bl::label(0x10000u, d, {}, kind).empty() && kind == OPENRAC_HOOK_UNKNOWN);
}

void into() {
    bl::set_device({true, bl::Pad::PlayStation});
    bl::set_key_names(vendor_keys());
    char out[8];
    CHECK(bl::label_into(OPENRAC_PAD_SQUARE, out, sizeof out) == OPENRAC_BUTTONS_KEYS);
    CHECK(std::strcmp(out, "F") == 0);
    char small[3];
    CHECK(bl::label_into(OPENRAC_PAD_CROSS, small, sizeof small) == OPENRAC_BUTTONS_KEYS);
    CHECK(std::strcmp(small, "SP") == 0);  // cut to fit
    CHECK(bl::label_into(OPENRAC_PAD_CROSS, nullptr, 8) == OPENRAC_HOOK_UNKNOWN);
    CHECK(bl::label_into(0, out, sizeof out) == OPENRAC_HOOK_UNKNOWN && out[0] == '\0');
    CHECK(bl::overlay_text(OPENRAC_UI_SQUARE " Buy all") == "[F] Buy all");
    CHECK(bl::overlay_text("No tokens") == "No tokens");
    bl::set_device({});
    bl::set_key_names({});
    CHECK(bl::label_into(OPENRAC_PAD_SQUARE, out, sizeof out) == OPENRAC_BUTTONS_PLAYSTATION);
    CHECK(std::strcmp(out, OPENRAC_UI_SQUARE) == 0);
}

}  // namespace

int main() {
    playstation();
    letters();
    keys();
    not_a_button();
    into();
    return openrac::test::result();
}
