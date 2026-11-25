// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// When the mods' pages of the game's menus may be on the screen and take the pad (mod_ui.h,
// menus_up): only in the boot program while the title stays. A party's Start from a mod's page
// sets the title's exit word; the movies that play next must have the pad (Start skips them), as
// they do after the game's own New Game. A mod's page on the pause menu (pause_row, pause_scroll):
// its rows' label and value, and the window of rows moving with the game's cursor. What a row of
// the options pages is pressed with (options_menu.h, row_press): the stick pushed up or down onto a
// row, which the game also makes Left or Right, steps nothing.

#include "check.h"
#include "mod_ui.h"
#include "options_menu.h"

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

    using openrac::frontend::options_menu::row_press;
    using openrac::frontend::options_menu::RowPress;
    constexpr std::uint32_t kCross = 0x0040, kRight = 0x2000, kLeft = 0x8000, kDown = 0x4000;
    // Cross, whatever the stick does; the d-pad's Left and Right.
    CHECK(row_press(kCross, kCross | kDown | kRight, 0.4f, 1.0f) == RowPress::Cross);
    CHECK(row_press(kRight, kRight, 0.0f, 0.0f) == RowPress::Right);
    CHECK(row_press(kLeft, kLeft, 0.0f, 0.0f) == RowPress::Left);
    // The stick pushed down a little to the right: the game makes it Down and Right; nothing.
    CHECK(row_press(0, kDown | kRight, 0.2f, 1.0f) == RowPress::None);
    CHECK(row_press(0, kLeft, -0.3f, -0.9f) == RowPress::None);
    // The stick pushed sideways: a step that way.
    CHECK(row_press(0, kRight, 1.0f, 0.1f) == RowPress::Right);
    CHECK(row_press(0, kLeft, -0.8f, 0.5f) == RowPress::Left);
    // Nothing pressed, or only Down.
    CHECK(row_press(0, 0, 0.0f, 0.0f) == RowPress::None);
    CHECK(row_press(0, kDown, 0.0f, 1.0f) == RowPress::None);
    return openrac::test::result();
}
