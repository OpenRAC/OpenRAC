// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mods' pages of the game's own menus and their items in the title's main menu (mod interface
// 1.7: openrac_api.ui_page, ui_main_item, ui_set ...; docs/port/MODS.md, section 6). What is on
// the pages is the loader's (mods.h, UiPage); this shows them with the game's widgets: in Ratchet &
// Clank the main menu's page gets a row per item under the game's three (the frame moby of a free
// slot playing the next row's seq), each item opens a copy of the Options page whose title, list
// and footer are the mod's page, and a page opened from there is a copy of the Language page, a
// level down. A page's frames are stretched sideways to its widest title, item and footer line,
// within the screen's safe area (menu_fit.h); the game's own pages keep theirs. The list is the game's text list, drawn by the game in its font; its cursor, Cross,
// Left, Right and Triangle are the port's (the widget has no update), with the game's sounds. A
// question is the memory card's prompt box in the list's place. In a level, the mods' items of the
// pause menu's Options page (1.9: ui_pause_item) open copies of the game's Camera page made by
// options_menu.cpp, whose title and rows this fills from the mod's page (pause_row, pause_scroll);
// there the game's widget has the cursor. A game without a front end (game_host.h,
// openrac_front_end) shows none.

#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace openrac::frontend::mod_ui {

// Whether the title's menus are up for the mods' pages: the boot program is loaded (not a level's,
// `boot_program`) and the title is not leaving for a level (its exit word, the front end's
// title_exit, still 0). Once a mod's page has started a game (a party's Start), the movies and the
// load that follow have the pad: no page of a mod's is on the screen any more, and none takes it.
inline bool menus_up(bool boot_program, int title_exit) {
    return boot_program && title_exit == 0;
}

// A row of a mod's page on the pause menu (openrac_api.ui_pause_item, since 1.9): the label on the
// left, a choice's value on the right (none for a button or a line of text). `is_choice` is whether
// the item is an OPENRAC_UI_CHOICE, `choice` its value among `choices`.
struct PauseRow {
    std::string label, value;
};

inline PauseRow pause_row(
    bool is_choice,
    const std::string& label,
    const std::vector<std::string>& choices,
    std::uint32_t choice
) {
    PauseRow row{label, ""};
    if (is_choice && !choices.empty()) {
        row.value = choices[std::min<std::size_t>(choice, choices.size() - 1)];
    }
    return row;
}

// The rows' window over a page's `count` items, `rows` shown, as the pause menu's Video page moves
// it: `top` the item in the first row, `cursor` the row the game's cursor is on. The cursor kept on
// a row there is; on the last row with items below, or the first with items above, the window moves
// by one and the cursor stays on the same item.
inline void pause_scroll(int& top, std::int32_t& cursor, int count, int rows) {
    top = std::clamp(top, 0, std::max(count - rows, 0));
    cursor = std::clamp<std::int32_t>(cursor, 0, std::max(std::min(count - top, rows) - 1, 0));
    if (cursor == rows - 1 && top + rows < count) {
        ++top;
        --cursor;
    } else if (cursor == 0 && top > 0) {
        --top;
        ++cursor;
    }
}

// The pause menu's Options items of the mods: the text ids of item `index`'s label, of the title of
// the page on the screen, and of its rows' first label (the rows' labels follow, then their values),
// for options_menu.cpp's pages; the strings are text()'s.
int pause_item_text(int index);
int pause_title_text();
int pause_row_text();

// Once a tick, after the game's: the main menu's items, the page on the screen, its input and its
// events, what the mods asked for. `loaded_overlay` is the program loaded (the boot program's menus
// only).
void update(int loaded_overlay);

// Pad 0's buttons as the game will read them (active low): while a mod's page is on the screen the
// port takes them, and the game sees none; on a pause page, only Triangle while the port has it
// (the page's own back, a question).
void filter_pad(std::uint16_t& buttons);

// The game's text lookup, asked for any id: the string of one of the pages' texts in game memory,
// the question in place of the memory card's prompt while one is open, or 0.
std::uint32_t text(int id);

}  // namespace openrac::frontend::mod_ui
