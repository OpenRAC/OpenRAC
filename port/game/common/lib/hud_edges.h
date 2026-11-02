/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * The level HUD in a picture wider than the game's, what every game whose HUD is the same 13 slots
 * shares (rac1/host/hud.c, rac2-ntsc/host/hud.c): each slot is 0x90 bytes, an element with a place
 * on the game's 512-pixel screen (+0x50 x) and anchor flags (+0x60: bit 0 top, bit 1 bottom, bit 2
 * left, bit 3 right; neither of a pair is centred).
 *
 * With the player's HUD setting on "Edges" (openrac_game_hud_side), the elements anchored to the
 * screen's left or right are drawn further out by the width the picture gained on that side;
 * centred ones stay in the middle. The place is moved only around the game's own draw and put back
 * after it (by the same amount, so whatever the draw itself did to it stays): nothing else in the
 * game sees another HUD.
 *
 * The idea is ReRAC's, which moves the same anchors out by the same flags (crates/rc-game/src/hud.rs,
 * `anchor`, and crates/rc-engine/src/display.rs, `side_extra`; ISC License, Copyright (c) 2026 ReRAC
 * contributors); no code of it is used.
 */
#pragma once

#include "openrac/game_host.h"
#include "openrac/guest.h"

/* Runs `draw`, the game's draw of the HUD whose slots are at `slots`, with the elements out. */
static inline void openrac_hud_edges(gaddr slots, void (*draw)(void)) {
    enum { SLOTS = 13, SLOT_BYTES = 0x90, SLOT_X = 0x50, SLOT_FLAGS = 0x60, ANCHOR_LEFT = 4, ANCHOR_RIGHT = 8 };
    const int side = openrac_game_hud_side();
    int moved[SLOTS];
    for (int i = 0; i < SLOTS; i++) {
        const gaddr slot = slots + (gaddr)i * SLOT_BYTES;
        const int flags = GREF(int, slot + SLOT_FLAGS);
        moved[i] = side == 0 ? 0 : (flags & ANCHOR_LEFT) != 0 ? -side : (flags & ANCHOR_RIGHT) != 0 ? side : 0;
        GREF(int, slot + SLOT_X) += moved[i];
    }
    draw();
    for (int i = 0; i < SLOTS; i++) {
        GREF(int, slots + (gaddr)i * SLOT_BYTES + SLOT_X) -= moved[i];
    }
}
