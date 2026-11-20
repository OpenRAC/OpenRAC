/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/* What ui2d.c shares with the level's HUD (hud.c): whose copy of the 2D layer draws, and its
 * helpers. */
#pragma once

#include <stdint.h>

#include "openrac/guest.h"

/* A program's copy of the 2D layer: the word holding its display list's cursor, the words holding
 * its HUD texture tables (by a texture's number a 4-byte entry, whose halfword at +2 indexes the
 * 8-byte image entries, +6 and +7 the log2 sizes), and the word holding the depth its HUD sprites
 * are drawn at (0: none). */
typedef struct Rac3Ui2d {
    gaddr cursor;
    gaddr hud_entries;
    gaddr hud_images;
    gaddr hud_depth;
} Rac3Ui2d;

/* The loaded level's copy (Veldin's addresses, relocated to the level's). */
Rac3Ui2d rac3_ui2d_level(void);
/* A 2D position (12.4) on the chip's screen, with the game's offset. */
int rac3_ui2d_screen_x(int sixteenths);
int rac3_ui2d_screen_y(int sixteenths);
/* A HUD texture's size in texels. */
void rac3_ui2d_hud_size(const Rac3Ui2d* u, int tex, int* tw, int* th);
/* A marker (an A+D write of `value` to the NOP register) at the list's cursor, the word at
 * `cursor`. */
void rac3_ui2d_marker(gaddr cursor, uint64_t value);
