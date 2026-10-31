/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Ratchet & Clank: Ratchet's animation, asked for a sequence his class does not have.
 *
 * Each level loads its own set of Ratchet's sequences (his class, moby +0x24: the count at +0xC,
 * the table at +0x48). Quartu has water but no drowning sequence (0x5F): reached without the O2
 * mask, running out of air asks for it, and set_hero_animation read through the missing entry
 * (issue #39; the game on the console crashes too). A sequence the class lacks is swapped for the
 * swimming one (0x65) when the class has that, and otherwise the animation is left as it is; the
 * state goes on as the game set it (drowning still kills him and he respawns).
 */
#include <stdio.h>

#include "rac1.h"

#define HERO_MOBY (RAC1_ADDR(0x0013F450u) + 0x2080u)
#define SWIM_SEQUENCE 0x65

static int has_sequence(gaddr pclass, int seq) {
    return seq >= 0 && seq < (int)GREF(uint8_t, pclass + 0xCu) && GREF(gaddr, pclass + 0x48u + (gaddr)seq * 4u) != 0;
}

void func_L00_00232C10(int seq, int frame, float time) {
    const gaddr moby = GREF(gaddr, HERO_MOBY);
    const gaddr pclass = moby != 0 ? GREF(gaddr, moby + 0x24u) : 0;
    if (pclass != 0 && !has_sequence(pclass, seq)) {
        static int logged = -1;
        const int fallback = has_sequence(pclass, SWIM_SEQUENCE) ? SWIM_SEQUENCE : -1;
        if (logged != seq) {
            char line[96];
            snprintf(line, sizeof(line), "Ratchet has no animation sequence %#x in this level; %s", seq,
                     fallback >= 0 ? "swimming instead" : "kept the current one");
            openrac_game_warn(line);
            logged = seq;
        }
        if (fallback < 0) {
            return;
        }
        seq = fallback;
    }
    func_L00_00232C10__game(seq, frame, time);
}
