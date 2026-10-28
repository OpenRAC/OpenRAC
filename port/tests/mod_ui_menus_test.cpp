// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// When the mods' pages of the game's menus may be on the screen and take the pad (mod_ui.h,
// menus_up): only in the boot program while the title stays. A party's Start from a mod's page
// sets the title's exit word; the movies that play next must have the pad (Start skips them), as
// they do after the game's own New Game.

#include "check.h"
#include "mod_ui.h"

int main() {
    using openrac::frontend::mod_ui::menus_up;
    CHECK(menus_up(true, 0));
    // The title leaving for a level (a start a mod's page asked for): the page lets go.
    CHECK(!menus_up(true, 1));
    // A level's program loaded: no title.
    CHECK(!menus_up(false, 0));
    CHECK(!menus_up(false, 1));
    return openrac::test::result();
}
