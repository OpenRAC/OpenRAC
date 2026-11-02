/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Going Commando: the level HUD in a picture wider than the game's. The same 13 slots as Ratchet &
 * Clank's (0x00255330), whose update func_0028E458 calls once a frame; func_0028B740 turns a slot's
 * place into a corner by the same flags. game/common/lib/hud_edges.h says how "HUD: Edges" moves them.
 *
 * The weapon, ammunition, health and bolts are not slots here: func_0034E4F8 draws them from a tree of
 * widgets with places of their own. With "HUD: Edges" two markers around it have the window move what
 * it draws left or right of the middle out to the picture's sides (renderer::GifInterpreter::
 * kHudSideMarker). A widget drawn across the middle stays; one crossing the middle third's edge would
 * come apart, and this HUD draws none in play.
 */
#include <stdint.h>

#include "../../common/lib/hud_edges.h"
#include "game_protos.h"

static void draw_hud(void) {
    func_0028E458__game();
}

void func_0028E458(void) {
    openrac_hud_edges(OPENRAC_DATA(0x00255330u), draw_hud);
}

enum { REG_NOP = 0x7F };

void func_0034E4F8(gaddr object) {
    const int side = openrac_game_hud_side();
    if (side != 0) {
        func_002FC560(REG_NOP, 0x4853ull << 48 | (uint32_t)side);
    }
    func_0034E4F8__game(object);
    if (side != 0) {
        func_002FC560(REG_NOP, 0x4853ull << 48);
    }
}
