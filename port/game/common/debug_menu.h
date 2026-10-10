// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The port's debug menu (not the game's): a level selector over the title and main menu, for
// testing. F1 (or Select on a pad) opens it while the boot program runs; Up and Down pick a level,
// Return / Space / Cross starts it, F1 / Backspace / Triangle closes it. The level is started the
// way --level N with OPENRAC_DIRECT does (game/rac1-pal/host/boot.c): NewGameInit for that level,
// then the menus closed and the start flag set, so the game loads it as a new game's first level.
//
// OPENRAC_DEBUG_MENU=FRAME opens it at that window frame (for scripted checks).

#pragma once

#include <cstdint>

#include <SDL3/SDL.h>

namespace openrac::frontend::debug_menu {

// A window event: true if the menu took it (the game does not see it).
bool event(const SDL_Event& event, const char* game_id, int loaded_overlay);

// Once a frame, before the game's frame is drawn: runs a level start the menu asked for.
void update(const char* game_id, int loaded_overlay, std::uint64_t frame);

// The game's pad 0 as the game will read it: while the menu is open its buttons move the menu
// and the game sees none pressed (active low: 0xFFFF).
void filter_pad(std::uint16_t& buttons, const char* game_id, int loaded_overlay);

bool is_open();

// Draws the menu over the finished frame (`framebuffer`, `width` x `height` pixels: the game's
// 512 x 448 picture scaled), if it is open.
void draw(unsigned framebuffer, int width, int height);

// GL objects, before the context goes.
void release();

}  // namespace openrac::frontend::debug_menu
