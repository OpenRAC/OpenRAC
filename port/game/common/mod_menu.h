// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mods' front-end menu entries and their pages (mod interface 1.5: openrac_api.menu_entry,
// menu_rect, menu_text; docs/port/MODS.md, section 6). In Ratchet & Clank each entry is an item of
// the title's Options list, after the port's own (Video, and Planets with developer tools), made as
// debug_menu.cpp makes Planets: the item opens a copy of the Language page. While that page is the
// one shown, the page is the mod's: the game sees no button of pad 0, the mod's callback runs once a
// frame with the pad's buttons and draws on a picture of 512 by 448 pixels (renderer/ui/canvas.h),
// which covers the whole window. When the callback says close, the port presses Triangle for the
// game, which goes back to the Options page as from any of its pages. A game without a front end
// (game_host.h, openrac_front_end) shows no entry.

#pragma once

#include <cstdint>

namespace openrac::frontend::mod_menu {

// Once a frame, after debug_menu::update: puts the entries in the Options list and runs the open
// page's callback. `loaded_overlay` is the program loaded (the entries are the boot program's).
void update(int loaded_overlay);

// Pad 0's buttons as the game will read them (active low): while a mod's page is open they go to
// the mod and the game sees none, but the Triangle that closes the page.
void filter_pad(std::uint16_t& buttons);

// Whether a mod's page is open.
bool is_open();

// Draws the open page over the finished frame (`framebuffer`, `width` x `height` pixels).
void draw(unsigned framebuffer, int width, int height);

// GL objects, before the context goes.
void release();

// The game's text lookup, asked for an id of the entries' own: the label in game memory, or 0 for
// any other id.
std::uint32_t text(int id);

// The host's drawing for the mods (mods::Host::menu_rect, menu_text): onto the open page.
void rect(int x, int y, int width, int height, std::uint32_t rgba);
void text_at(int x, int y, int size, std::uint32_t rgba, const char* text);

}  // namespace openrac::frontend::mod_menu
