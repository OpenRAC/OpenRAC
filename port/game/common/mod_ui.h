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
// question is the memory card's prompt box in the list's place. A game without a front end
// (game_host.h, openrac_front_end) shows none.

#pragma once

#include <cstdint>

namespace openrac::frontend::mod_ui {

// Whether the title's menus are up for the mods' pages: the boot program is loaded (not a level's,
// `boot_program`) and the title is not leaving for a level (its exit word, the front end's
// title_exit, still 0). Once a mod's page has started a game (a party's Start), the movies and the
// load that follow have the pad: no page of a mod's is on the screen any more, and none takes it.
inline bool menus_up(bool boot_program, int title_exit) {
    return boot_program && title_exit == 0;
}

// Once a tick, after the game's: the main menu's items, the page on the screen, its input and its
// events, what the mods asked for. `loaded_overlay` is the program loaded (the boot program's menus
// only).
void update(int loaded_overlay);

// Pad 0's buttons as the game will read them (active low): while a mod's page is on the screen the
// port takes them, and the game sees none.
void filter_pad(std::uint16_t& buttons);

// The game's text lookup, asked for any id: the string of one of the pages' texts in game memory,
// the question in place of the memory card's prompt while one is open, or 0.
std::uint32_t text(int id);

}  // namespace openrac::frontend::mod_ui
