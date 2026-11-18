/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Going Commando's host C, one copy for every disc (USA v1.01, PAL, USA v2.00; port/game/rac2/README.md).
 *
 * The files here are compiled into each release's program (hostgen.json, "host_shared") with that
 * release's translation and its tables (port/game/rac2-<release>/host). They are written with v1.01's
 * names for the game's functions (func_0027F538, func_L05_0041DF58, func_0027F640__game), which another
 * release's build writes as its own before compiling (rac2_names.tsv, tools/hostgen/release_host.py),
 * and its addresses and constants inside RAC2_ADDR(), RAC2_LADDR() and RAC2_K(), which rac2_version.h
 * turns into the release's: a fix written here holds for every disc.
 */
#ifndef OPENRAC_RAC2_H
#define OPENRAC_RAC2_H

/* A release's addresses, constants and layout. */
#include "rac2_version.h"

#if !defined(RAC2_ADDR) || !defined(RAC2_LADDR) || !defined(RAC2_K) || !defined(RAC2_LEVEL_BASE)
#error "rac2_version.h must define RAC2_ADDR, RAC2_LADDR and the release's layout"
#endif

/* A function's name in this release, as a string (for logs and tables that name functions). */
#define RAC2_NAME_(f) #f
#define RAC2_NAME(f) RAC2_NAME_(f)

#include <stdint.h>

/* The levels, by the game's own numbers (tools.c's names; NULL for a number with no program). */
#define RAC2_LEVELS 31
extern const char* const rac2_level_names[RAC2_LEVELS];
/* A level started as the developer tools start one (tools.c): from the title a new game's save
 * first when `new_game`, the level held as the destination until its program is in; from a level,
 * RequestLevelExit to it. */
void rac2_start_level(int level, int new_game);
/* The programs loaded since the start: each level's and the title's (level.c, ParseBin). */
uint32_t rac2_program_loads(void);
/* Once a frame, before the mods' tick (tools.c's on_frame): what the play calls follow over time
 * (play.c). */
void rac2_play_frame(void);
/* The play calls' table installed (play.c, from rac2_install_tools). */
void rac2_install_play(void);
/* The mods' pages in the title's main menu (mod_pages.c): the table installed (from
 * rac2_install_tools); the label of a text id of theirs, or 0; whether one of them is drawn by the
 * 16:9 page's object now (its update then only moves the panel), and its draw, the page's title put
 * in `title` (options.c's wraps). */
void rac2_install_mod_pages(void);
uint32_t rac2_mod_pages_text(int id);
int rac2_mod_pages_tick(void);
int rac2_mod_pages_draw(char* title, unsigned title_bytes);
/* The pause menu's (options.c's Options list, after Video): the mods' items shown (at most
 * OPENRAC_UI_PAUSE_ITEMS_MAX), item `index`'s label's and description's text ids, its page (a copy
 * of the Camera page's description) made known, and the text a pause page's title is drawn from. */
int rac2_mod_pause_items(void);
int rac2_mod_pause_label_text(int index);
int rac2_mod_pause_help_text(int index);
void rac2_mod_pause_page(int index, uint32_t page);
uint32_t rac2_mod_pause_title(void);

#endif /* OPENRAC_RAC2_H */
