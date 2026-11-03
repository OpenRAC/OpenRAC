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
 * kHudSideMarker). What overlaps moves as one element; one across the middle stays, and so does the
 * top middle panel (func_0034AD10).
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

enum {
    REG_NOP = 0x7F
};

static void side_marker(int side) {
    if (side != 0) {
        func_002FC560(REG_NOP, 0x4853ull << 48 | (uint32_t)side);
    }
}

static int widgets_side;

void func_0034E4F8(gaddr object) {
    const int side = openrac_game_hud_side();
    widgets_side = side;
    side_marker(side);
    func_0034E4F8__game(object);
    if (side != 0) {
        func_002FC560(REG_NOP, 0x4853ull << 48);
    }
    widgets_side = 0;
}

/*
 * The panel in the top middle, drawn among those widgets: its tip overlaps the health widget's, which
 * would make them one element, left of the middle. It is kept out of what the markers move.
 */
void func_0034AD10(gaddr panel) {
    const int side = widgets_side;
    if (side != 0) {
        func_002FC560(REG_NOP, 0x4853ull << 48);
    }
    func_0034AD10__game(panel);
    side_marker(side);
}

/*
 * The ship levels draw a HUD of their own from a callback (func_0029B758), which is not the widgets
 * above: Feltzin, Hrugis and Gorn. The same markers move it.
 */
#define SHIP_HUD(name)                                                                             \
    int name(void) {                                                                               \
        const int side = openrac_game_hud_side();                                                  \
        side_marker(side);                                                                         \
        const int result = name##__game();                                                         \
        if (side != 0) {                                                                           \
            func_002FC560(REG_NOP, 0x4853ull << 48);                                               \
        }                                                                                          \
        return result;                                                                             \
    }

SHIP_HUD(func_L05_0041DF58)
SHIP_HUD(func_L10_00421790)
SHIP_HUD(func_L15_0042E1B0)
/* Giant Clank's fights draw theirs the same way: his panel and the enemy's go to the sides. */
SHIP_HUD(func_L22_003A5018)
SHIP_HUD(func_L23_0041DA48)
