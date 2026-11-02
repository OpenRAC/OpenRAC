/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Ratchet & Clank: the level HUD in a picture wider than the game's. The HUD is 13 slots (D_00199C60)
 * whose draw callbacks func_001FFFB8 calls once a frame; game/common/lib/hud_edges.h says how the
 * player's "HUD: Edges" moves them. Every element draws from its +0x50: the bolt counter and the
 * nanotech straight from it, the meters through func_L00_00236400, which turns it into a corner by
 * the flags.
 */
#include "../../common/lib/hud_edges.h"
#include "rac1.h"

static void draw_hud(void) {
    func_001FFFB8__game();
}

void func_001FFFB8(void) {
    openrac_hud_edges(OPENRAC_DATA(RAC1_ADDR(0x00199C60u)), draw_hud);
}
