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

/* The id of the next world quad drawn (openrac_game_quad::id), counted up per quad; 0 for none.
 * A frame between ticks moves a quad with an id from where the last tick had it. */
extern uint32_t rac2_effect_quad_id;

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

/* The Shield Charger's and the Tesla Barrier's moby (tesla_barrier_update, func_L00_003C3D90: it
 * sits on the hero, and its draw callback lays the shield's glow sphere and arcs around him). */
#define RAC2_SHIELD_CHARGER_CLASS 2652
#define RAC2_TESLA_BARRIER_CLASS 4498

/* What a draw callback's strips follow between ticks: a moby, and whether only its position (its
 * world offset kept, not turned with it). */
typedef struct rac2_strip_anchor {
    gaddr moby;
    int position_only;
} rac2_strip_anchor;

/* The anchor of the strips the callback given `owner` lays out, for the display list sent a tick
 * late (wide_view.c): the nanotech ball's (rac2_effect_attachment, turned with it), and those drawn
 * on the hero (`hero`, his moby), the weapon in his hand (`hand`, its moby) and his shield, which
 * follow their position only. Laid out a tick before the mobys drawn with them, they were left a
 * tick's travel behind the hero: on Siberius's cart the shield dragged behind him and his weapons'
 * beams started off the gun. Others (trails and beams in the world) stay where the tick has them. */
static inline rac2_strip_anchor rac2_strip_anchor_of(gaddr owner, gaddr hero, gaddr hand) {
    rac2_strip_anchor anchor = {rac2_effect_attachment(owner), 0};
    if (anchor.moby != 0 || owner == 0 || owner > 0x01FFFF00u) {
        return anchor;
    }
    if (owner == hero || owner == hand) {
        anchor.moby = owner;
        anchor.position_only = 1;
        return anchor;
    }
    uint16_t class_id;
    memcpy(&class_id, G(owner + 0xAAu), sizeof(class_id));
    /* A live moby of the shield's classes (state +0x20 under 0x80). */
    if ((class_id == RAC2_SHIELD_CHARGER_CLASS || class_id == RAC2_TESLA_BARRIER_CLASS)
        && GREF(uint8_t, owner + 0x20u) < 0x80u) {
        anchor.moby = owner;
        anchor.position_only = 1;
    }
    return anchor;
}

#endif
