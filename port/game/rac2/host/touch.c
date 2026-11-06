/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Going Commando: the on-screen pad's context (game_host.h, openrac_game_touch), read from the
 * game's memory once a frame: which of the sets in game/common/touch/sets/ the pad shows. It only
 * reads.
 *
 * Outside a level the title and its menus ("front_end"). In a level its game state
 * (g_nGameState, as rac1's game mode): 0 play, 3 the pause menu ("menu"), 4 a dialog ("dialog"),
 * 5 a vendor ("vendor", OpenVendorMenu), anything else a movie, a scene or a load ("cutscene").
 * In play the space combat levels (Feltzin System, Hrugis Cloud, Gorn) are the ship's, the rest on
 * foot. Installed with the tools (tools.c, rac2_install_tools).
 */
#include "openrac/game_host.h"
#include "openrac/host_env.h"
#include "rac2.h"

#define LEVELS 31
#define LEVEL_FELTZIN 5
#define LEVEL_HRUGIS 10
#define LEVEL_GORN 15

static const char* play_context(int level) {
    if (level == LEVEL_FELTZIN || level == LEVEL_HRUGIS || level == LEVEL_GORN) {
        return "ship";
    }
    return GREF(uint32_t, RAC2_g_pHeroMoby) != 0 ? "foot" : "cutscene";
}

int rac2_touch(const char** context, const char* hints[OPENRAC_TOUCH_HINTS_MAX]) {
    (void)hints;
    const int level = openrac_game_loaded_overlay();
    if (level < 0 || level >= LEVELS) {
        *context = "front_end";
        return 0;
    }
    switch (GREF(int32_t, OPENRAC_DATA(RAC2_g_nGameState))) {
        case 0:
            *context = play_context(level);
            return 0;
        case 3:
            *context = "menu";
            return 0;
        case 4:
            *context = "dialog";
            return 0;
        case 5:
            *context = "vendor";
            return 0;
        default:
            *context = "cutscene";
            return 0;
    }
}
