// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The port's Exit item: it closes the game, after asking. In Ratchet & Clank the title's main menu
// gets an Exit row under the game's (and the mods') rows, which opens a page asking "Exit?" with
// Yes and No (a copy of the Options page with the Language page's title and list, as a mod's page
// is); the pause menu's Options page gets an Exit item after Quit Game, whose list then asks in
// place (options_menu.cpp). The answers are the game's own list, moved by the pad as the game's
// menus are; Cross on Yes closes the window as its close button does (an ordinary end, exit code 0:
// no crash report, nothing for the launcher to report), No or Triangle goes back. Texts in the
// game's language (exit_words.h). A game without a front end (game_host.h, openrac_front_end) shows
// none.

#pragma once

#include <cstdint>

#include "exit_words.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"

namespace openrac::frontend::exit_menu {

// A text's id for the game's text lookup (past the game's ids, under 0x7000, and beside the mods'
// pages' and the options page's: 0x7C00 and 0x7E00 on).
int text_id(Text text);

// Makes the texts' room in game memory, outside the game's text lookup (which then only fills it).
void prepare();

// Returns the address of text `id` in the game's language now, written into game memory, or 0 for
// an id that is not one of the texts above. The game's text lookup asks it for every id.
std::uint32_t text(int id);

// Writes at `at` a text list item labelled `label` with an action the game ignores (0x7F, drawn as
// any item): the port answers Cross on it.
void put_item(gaddr at, Text label);

// Plays the menus' confirm sound on the moby of text list `widget`.
void cue(const openrac_front_end& fe, gaddr widget);

// A Cross press on a page the port answers. A press counts once Cross was let go while the page
// was ready, so the press that opened the page is not taken, nor one a later frame reads again.
class CrossPress {
public:
    // Returns true for a new press of Cross on the first pad. `ready`: the page is settled with
    // its list in hand; false forgets any release seen.
    bool take(const openrac_front_end& fe, bool ready);

private:
    bool m_armed = false;
};

// The main menu's question page, made the first time (from the boot program's Options page).
gaddr title_page(const openrac_front_end& fe);

// Once a tick while the boot program runs: Cross on the question page's Yes or No.
void update_title(const openrac_front_end& fe);

// The game closed, as the window's close button closes it: logged with `where`, the window's
// quit event sent (the frame loop ends the program normally).
void quit(const char* where);

}  // namespace openrac::frontend::exit_menu
