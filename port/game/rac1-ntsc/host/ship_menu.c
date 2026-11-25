/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 * The ship computer's list in a level. PAL's catalogue folds the levels' copies of the list's
 * builder into func_00222848, so rac1/host/destinations.c wraps them all; the NTSC decomp names
 * the levels' copy apart (FUN_L00_0027f448). This sends it through the same wrap, and the wrap runs
 * the copy that was called. */
#include "game_protos.h"
#include "openrac/game_host.h"

static int in_level_copy;

/* RAC1_SHIP_MENU_CALL (rac1_version.h): the game's own list builder, the level's or the
 * executable's. */
int rac1_ntsc_ship_menu_game(gaddr menu, int unused) {
    if (in_level_copy) {
        return game_FUN_L00_0027f448__game(menu);
    } else {
        game_FUN_00221800__game(menu, unused);
    }
    return 0;
}

/* The NTSC decomp declares this copy int (menu); the screen's enter passes a second word the
 * builder never reads. Both copies return 0. */
int game_FUN_L00_0027f448(gaddr menu) {
    in_level_copy = 1;
    game_FUN_00221800(menu, 0);
    in_level_copy = 0;
    return 0;
}
