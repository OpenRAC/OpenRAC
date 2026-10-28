// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The port's Exit item: it closes the game, after asking. In Ratchet & Clank the title's main menu
// gets an Exit row under the game's (and the mods') rows, which opens a page asking "Exit?" with
// Yes and No (a copy of the Language page, as the developer menu's Planets page is); the pause
// menu's Options page gets an Exit item after Quit Game, whose list then asks in place
// (options_menu.cpp). The answers are the game's own list, moved by the pad as the game's menus
// are; Cross on Yes closes the window as its close button does (an ordinary end, exit code 0: no
// crash report, nothing for the launcher to report), No or Triangle goes back. Texts in the game's
// language (exit_words.h). A game without a front end (game_host.h, openrac_front_end) shows none.

#pragma once

#include <cstdint>

#include "exit_words.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"

namespace openrac::frontend::exit_menu {

// A text's id for the game's text lookup (past the game's ids, under 0x7000, and beside the mods'
// pages' and the options page's: 0x7C00 and 0x7E00 on).
int text_id(Text text);

// The game's text lookup, asked for any id: one of the texts above in the game's language now, in
// game memory, or 0.
std::uint32_t text(int id);

// The main menu's question page, made the first time (from the boot program's Language page).
gaddr title_page(const openrac_front_end& fe);

// Once a tick while the boot program runs: Cross on the question page's Yes or No.
void update_title(const openrac_front_end& fe);

// The game closed, as the window's close button closes it: logged with `where`, the window's
// quit event sent (the frame loop ends the program normally).
void quit(const char* where);

}  // namespace openrac::frontend::exit_menu
