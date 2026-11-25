/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/*
 * The bolt counter in the HUD past 9,999,999 (bolt_totals.h; the Load page's is in save_marks.c).
 *
 * func_0034CEA0 (UpdateItemStatusHud, src/game/0034CEA0-menu-g8.c) formats the shown total into
 * the HUD's text with "%d", and sizes the counter's panel for at most seven digits. The wrapper
 * lets the game do both, then writes the compact form over that text: the panel is already at its
 * widest, and a right-aligned "12.3M" sits inside it. func_0034DB98 (DrawBoltCounterPanel) draws
 * a thousands mark between each three digits of the player's total; while the compact form is up
 * it draws with a total of zero, which has none. Both calls are native text and quad drawing.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bolt_totals.h"
#include "game_protos.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "rac2.h"

#define HUD_BOLT_TEXT RAC2_K(HUD_BOLT_TEXT, 0x21C, func_0034CEA0)
#define PLAYER_BOLTS RAC2_ADDR(0x001A79F8u)

static int compact_shown;

void func_0034CEA0(gaddr hud) {
    func_0034CEA0__game(hud);
    char* text = (char*)G(hud + HUD_BOLT_TEXT);
    char* end = text;
    const long shown = strtol(text, &end, 10);
    compact_shown = 0;
    if (end != text && *end == '\0' && shown <= INT32_MAX) {
        compact_shown = rac2_bolts_compact(text, RAC2_BOLTS_COMPACT_SIZE, (int32_t)shown);
    }
}

void func_0034DB98(gaddr panel) {
    if (!compact_shown) {
        func_0034DB98__game(panel);
        return;
    }
    int32_t* bolts = (int32_t*)G(OPENRAC_DATA(PLAYER_BOLTS));
    const int32_t saved = *bolts;
    *bolts = 0;
    func_0034DB98__game(panel);
    *bolts = saved;
}
