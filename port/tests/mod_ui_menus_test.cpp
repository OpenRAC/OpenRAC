// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// When the mods' pages of the game's menus may be on the screen and take the pad (mod_ui.h,
// menus_up): only in the boot program while the title stays. A party's Start from a mod's page
// sets the title's exit word; the movies that play next must have the pad (Start skips them), as
// they do after the game's own New Game. A mod's page on the pause menu (pause_row, pause_scroll):
// its rows' label and value, and the window of rows moving with the game's cursor.

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

    using openrac::frontend::mod_ui::pause_row;
    using openrac::frontend::mod_ui::pause_scroll;
    // A choice: the label and its value (the last when past the end); a button or a line: no value.
    const std::vector<std::string> ping = {"38 ms, UDP"};
    CHECK(pause_row(true, "Flavy (host)", ping, 0).value == "38 ms, UDP");
    CHECK(pause_row(true, "Flavy (host)", ping, 4).value == "38 ms, UDP");
    CHECK(pause_row(true, "Code", {}, 0).value.empty());
    CHECK(pause_row(false, "Leave party", ping, 0).label == "Leave party");
    CHECK(pause_row(false, "Leave party", ping, 0).value.empty());
    CHECK(pause_row(false, "Not in a party", {}, 0).value.empty());
    // Five rows over seven items: the cursor down to the last row moves the window by one.
    int top = 0;
    std::int32_t cursor = 4;
    pause_scroll(top, cursor, 7, 5);
    CHECK(top == 1 && cursor == 3);
    cursor = 4;
    pause_scroll(top, cursor, 7, 5);
    CHECK(top == 2 && cursor == 3);
    cursor = 4;
    pause_scroll(top, cursor, 7, 5);  // the last item: no further
    CHECK(top == 2 && cursor == 4);
    cursor = 0;
    pause_scroll(top, cursor, 7, 5);  // back up from the first row
    CHECK(top == 1 && cursor == 1);
    // Fewer items than rows: the cursor kept on them, no window to move.
    top = 3;
    cursor = 4;
    pause_scroll(top, cursor, 2, 5);
    CHECK(top == 0 && cursor == 1);
    // No items: row 0.
    cursor = 3;
    pause_scroll(top, cursor, 0, 5);
    CHECK(top == 0 && cursor == 0);
    return openrac::test::result();
}
