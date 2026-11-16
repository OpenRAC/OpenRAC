/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Going Commando: whose draw callback is running. The draw callback lists (wide_view.c runs them)
 * call each function with its argument, for most a moby: the ship's engine flame in the space
 * fights (level 5's func_L05_004186D8, on the ship's moby) and the other effects a moby draws for
 * itself. The world quads drawn meanwhile (render.c) carry it, so that a frame between ticks can move
 * those on the moby's body with it (openrac_game_quad::parent; the window picks which).
 */
#ifndef OPENRAC_RAC2_DRAW_OWNER_H
#define OPENRAC_RAC2_DRAW_OWNER_H

#include <string.h>

#include "openrac/guest.h"

/* The running callback's argument, 0 outside the lists' runs. */
extern gaddr rac2_draw_owner;

/* Returns the nanotech ball's position anchor while its body is drawn, otherwise 0.
 * Class +0xAA and state +0x20 are shared by the releases
 * (seen in game code: func_003136D0 and func_003146C0). */
static inline gaddr rac2_effect_attachment(gaddr owner) {
    /* Callback arguments need not be mobys; reject addresses outside a complete RAM record. */
    if (owner == 0 || owner > 0x01FFFF00u) {
        return 0;
    }

    uint16_t class_id;
    /* Non-moby callback arguments need not be aligned for a 16-bit load. */
    memcpy(&class_id, G(owner + 0xAAu), sizeof(class_id));

    /* Only nanotech balls emit the matching body and outline (seen in game code: func_003146C0). */
    if (class_id != 806) {
        return 0;
    }

    /* Intact and floating balls draw a body; fly-in particles do not (seen in game code: func_003136D0). */
    const uint8_t state = GREF(uint8_t, owner + 0x20u);
    if (state != 1 && state != 2) {
        return 0;
    }

    return owner;
}

#endif
