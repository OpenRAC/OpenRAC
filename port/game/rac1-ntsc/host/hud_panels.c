/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
#include <stdint.h>
#include "game_protos.h"
#include "openrac/game_host.h"

/* Draw a texture slice without stretching its rounded corners. The game's
 * textured-quad path also supplies the packet fallback when native UI is off. */
static void panel_slice(unsigned long long texture, int x, int y, int w, int h,
                        int u, int v, int uw, int vh, int alpha) {
    const long long colour = ((long long)alpha << 24) | 0x7F7F7F;
    game_FUN_001f5450(x, y, w, h, u, v, uw, vh, colour, (long long)texture);
}

/* The optional HUD setting uses one capsule for the complete health layout. The normal
 * game draws two partly overlapping strips; switching the setting off keeps that code intact.
 * Reuse its textures, horizontal opening animation and opacity, and leave every
 * orb, glow and health animation to the game. */
int game_FUN_L00_00239810(gaddr element) {
    if (!*openrac_game_fix("fix_nanotech_hud_panel")) {
        return game_FUN_L00_00239810__game(element);
    }
    const int vehicle = GREF(uint8_t, 0x0013F350u + 0x20A4);
    const int capacity = vehicle == 1 ? GREF(int, 0x0013F350u + 0x22AC)
                                     : GREF(int, 0x0015EDA0u);
    const int visible = GREF(uint8_t, element + 0x70);
    const int limit = GREF(int, OPENRAC_LDATA(0, 0x0015F7F0u));
    const gaddr cap_address = OPENRAC_LDATA(0, 0x0015F81Cu);
    const int cap = GREF(int, cap_address);
    if (vehicle == 2 || GREF(int, 0x0013F350u + 0x2084) == 0x32
        || visible == 0 || limit <= 0 || cap <= 0
        || (capacity != 4 && capacity != 5 && capacity != 8)) {
        return game_FUN_L00_00239810__game(element);
    }
    const float fraction = visible < limit ? (float)visible / (float)limit : 1.0f;
    const int half_width = (int)((capacity == 5 ? 32 : 48) * fraction);
    const int padding = 4;
    const int height = cap + (capacity > 4 ? 32 : 0) + 2 * padding;
    const int x = GREF(int, element + 0x50);
    const int y = GREF(int, OPENRAC_LDATA(0, 0x0015F808u))
                  + (GREF(int, 0x0015ED80u) ? 10 : 18)
                  + GREF(int, OPENRAC_LDATA(0, 0x0015F810u)) - padding;
    const int alpha = (int)(GREF(int, OPENRAC_LDATA(0, 0x0015F820u)) * fraction);
    const int end = game_FUN_001ff960(0x7580, 1);
    const int middle = game_FUN_001ff960(0x7580, 0);
    if (end < 0 || middle < 0) {
        return game_FUN_L00_00239810__game(element);
    }
    const unsigned long long cap_texture = game_FUN_001ffa10(end);
    const unsigned long long body_texture = game_FUN_001ffa10(middle);
    const int tw = 1 << ((cap_texture >> 26) & 15);
    const int th = 1 << ((cap_texture >> 30) & 15);
    const int bw = 1 << ((body_texture >> 26) & 15);
    const int bh = 1 << ((body_texture >> 30) & 15);
    const int top = cap / 2;
    /* Sample the solid centre row for the body and straight sides. Split the
     * original caps into corners of fixed height: a taller panel adds straight
     * sides, not taller ellipses or extra transparent texture padding. */
    panel_slice(body_texture, x - half_width, y, half_width * 2, top,
                0, 0, bw, bh / 2, alpha);
    panel_slice(body_texture, x - half_width, y + top, half_width * 2, height - cap,
                0, bh / 2, bw, 1, alpha);
    panel_slice(body_texture, x - half_width, y + height - (cap - top),
                half_width * 2, cap - top, 0, bh / 2, bw, bh - bh / 2, alpha);
    for (int right = 0; right < 2; ++right) {
        const int edge = right ? x + half_width : x - half_width - cap;
        const int u = right ? tw : 0;
        const int uw = right ? -tw : tw;
        panel_slice(cap_texture, edge, y, cap, top, u, 0, uw, th / 2, alpha);
        panel_slice(cap_texture, edge, y + top, cap, height - cap,
                    u, th / 2, uw, 1, alpha);
        panel_slice(cap_texture, edge, y + height - (cap - top), cap, cap - top,
                    u, th / 2, uw, th - th / 2, alpha);
    }

    /* Zero-height original background quads draw nothing. Only this drawing
     * call sees the temporary size; all orb placement and game data survive. */
    GREF(int, cap_address) = 0;
    const int result = game_FUN_L00_00239810__game(element);
    GREF(int, cap_address) = cap;
    return result;
}
