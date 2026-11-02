/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * The 2D layer's quads drawn natively, what every game's wrapped draw routines share (rac1/host/
 * ui2d.c, rac2-ntsc/host/ui2d.c): a quad goes to the window as a record (openrac_game_ui_quad) with
 * the corners and texel coordinates its packet would have held, and the display list gets a marker
 * in its place, so the window draws it where it came.
 */
#pragma once

#include <stdint.h>

#include "openrac/game_host.h"
#include "openrac/guest.h"

/* The quad to the window and its marker into the display list whose cursor is the word at
 * `cursor` (DMA cnt and VIF DIRECT of two quadwords, a PACKED GIF tag of one A+D register, the A+D
 * write to NOP carrying the index); false when the 2D layer is replayed, and the caller sends the
 * game's own packet. */
static inline int openrac_ui_send(const openrac_game_ui_rect* q, gaddr cursor) {
    const int index = openrac_game_ui_quad(q);
    if (index < 0) {
        return 0;
    }
    const gaddr at = GREF(gaddr, cursor);
    GREF(uint32_t, at + 0) = 0x10000002u;
    GREF(uint32_t, at + 4) = 0;
    GREF(uint32_t, at + 8) = 0;
    GREF(uint32_t, at + 12) = 0x50000002u;
    GREF(uint64_t, at + 16) = 1ull | (1ull << 15) | (1ull << 60);
    GREF(uint64_t, at + 24) = 0xEull;
    GREF(uint64_t, at + 32) = (0x4E55ull << 48) | (uint32_t)index;
    GREF(uint64_t, at + 40) = 0x7Full;
    GREF(gaddr, cursor) = at + 48;
    return 1;
}

/* An axis-aligned quad, (x0, y0) to (x1, y1) on the screen (12.4, offset as the game sends them)
 * and (u0, v0) to (u1, v1) in the texture (12.4), as a strip. */
static inline void openrac_ui_rect(
    openrac_game_ui_rect* q, int x0, int y0, int x1, int y1, int u0, int v0, int u1, int v1
) {
    q->x[0] = (uint32_t)x0, q->y[0] = (uint32_t)y0, q->u[0] = (uint32_t)u0, q->v[0] = (uint32_t)v0;
    q->x[1] = (uint32_t)x1, q->y[1] = (uint32_t)y0, q->u[1] = (uint32_t)u1, q->v[1] = (uint32_t)v0;
    q->x[2] = (uint32_t)x0, q->y[2] = (uint32_t)y1, q->u[2] = (uint32_t)u0, q->v[2] = (uint32_t)v1;
    q->x[3] = (uint32_t)x1, q->y[3] = (uint32_t)y1, q->u[3] = (uint32_t)u1, q->v[3] = (uint32_t)v1;
}
