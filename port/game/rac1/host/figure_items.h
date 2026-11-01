/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Ratchet & Clank: what figure_items.c (the pieces a figure wears) and figure_shots.c (the shots it
 * fires) share.
 */
#ifndef RAC1_FIGURE_ITEMS_H
#define RAC1_FIGURE_ITEMS_H

#include "openrac/host_env.h"
#include "rac1.h"

/* The hero (play.c): his moby, and the level context (0 while the game plays a level). */
#define HERO RAC1_ADDR(0x0013F450u)
#define HERO_MOBY (HERO + 0x2080u)
#define CONTEXT OPENRAC_LDATA(0, RAC1_LADDR(00, 0x0015F808u))

/* The hero (play.c), his item slots, his attach matrices. */
#define HERO_SLOTS (HERO + 0x1090u)
#define SLOT_SIZE 0x50u
#define SLOT_THROWN 0x1Au
#define SLOT_ITEM 0x28u
#define SLOTS 7

/* The fields of a moby these read. */
enum {
    MOBY_POS = 0x10,
    MOBY_STATE = 0x20,
    MOBY_CLASS = 0x24,
    MOBY_SCALE = 0x2C,
    MOBY_UPDATE_DIST = 0x30,
    MOBY_VISIBLE = 0x31,
    MOBY_DRAW_DIST = 0x32,
    MOBY_FLAGS = 0x34,
    MOBY_COLOUR = 0x38,
    MOBY_ROT = 0x40,
    MOBY_FRAME = 0x50,
    MOBY_SEQ = 0x52,
    MOBY_ANIM_TIME = 0x54,
    MOBY_CUR_FRAME = 0x68,
    MOBY_PREV_FRAME = 0x6C,
    MOBY_UNK73 = 0x73,
    MOBY_UPDATE = 0x74,
    MOBY_PVARS = 0x78,
    MOBY_LIGHT = 0x80,
    MOBY_COLLISION = 0x94,
    MOBY_COLLISION_OFF = 0x98,
    MOBY_OCLASS = 0xA6,
    MOBY_MATRIX = 0xC0
};

/* A moby of `oclass` to show a figure's shot with: its look, never run, colliding with nothing; 0
 * when the class cannot be had or the level has no room. */
gaddr rac1_figure_track_moby(gaddr figure, int32_t oclass);
void rac1_figure_track_moby_place(gaddr moby, const float* position);
void rac1_figure_track_moby_drop(gaddr moby, int32_t oclass);
/* Whether the game can make a moby of `oclass` now, with nothing decoded for it. */
int rac1_figure_class_ready(int32_t oclass);

/* Whether OPENRAC_ITEMS_DEBUG says what the hero holds, the pieces and the shots on stderr. */
int rac1_figure_items_debug(void);

#endif
