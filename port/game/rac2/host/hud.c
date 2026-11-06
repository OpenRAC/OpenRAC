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

#include "game_protos.h"
#include "rac2.h"
#include "../../common/lib/hud_edges.h"

static void draw_hud(void) {
    func_0028E458__game();
}

void func_0028E458(void) {
    openrac_hud_edges(OPENRAC_DATA(RAC2_ADDR(0x00255330u)), draw_hud);
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
/* The name is expanded first, so that a release's table (rac2_version.h) renames it before ## pastes. */
#define SHIP_HUD(name) SHIP_HUD_(name)
#define SHIP_HUD_(name)                                                                            \
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
/*
 * The hoverbike races (Barlow, Joba): before the start, during the race and at the end; and the
 * crane on Endako, a callback of func_0029B7C0, whose camera's frame goes round the screen. With
 * the stretch bit (renderer::GifInterpreter::kHudStretch) what crosses the middle grows out to the
 * sides with the rest.
 */
#define STRETCH (1ull << 32)

#define RACE_HUD(name)                                                                             \
    void name(void) {                                                                              \
        const int side = openrac_game_hud_side();                                                  \
        if (side != 0) {                                                                           \
            func_002FC560(REG_NOP, 0x4853ull << 48 | STRETCH | (uint32_t)side);                     \
        }                                                                                          \
        name##__game();                                                                            \
        if (side != 0) {                                                                           \
            func_002FC560(REG_NOP, 0x4853ull << 48);                                               \
        }                                                                                          \
    }

RACE_HUD(func_L04_0040AD28)
RACE_HUD(func_L04_00411298)
RACE_HUD(func_L04_00417308)
RACE_HUD(func_L03_003C7018)

/* The meter func_00290010 shows for a while (the Thermanator's, on what it freezes or thaws), a
 * callback of func_0029B7C0: drawn right of the middle, it goes out to that side. */
SHIP_HUD(func_002900B8)
