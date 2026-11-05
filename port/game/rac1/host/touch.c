/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Ratchet & Clank: the on-screen pad's context (game_host.h, openrac_game_touch), read from the
 * game's memory once a frame: which of the sets in game/common/touch/sets/ the pad shows. One
 * table for every disc: the addresses are games/rac1's, each version's through its rac1_version.h.
 *
 * In the boot program, the title and its menus ("front_end"), or the flight between planets and a
 * load ("cutscene"). In a level its game mode (D_L00_0015F6A8, play.c): 0 play, 1 a movie, 2 a
 * cutscene, 6 the ship's landing or take-off and 7 a slideshow ("cutscene"), 3 the pause menu
 * ("menu"), 4 a dialog ("dialog"), 5 a vendor ("vendor"). In play the hero's vehicle mode (+0x20A4:
 * 1 Clank, 2 Giant Clank; include/rnc/hero/hero.h) and his state (+0x2084, rnc/hero/hero_state.h):
 * swimming, grinding, the hoverboard. It only reads. Installed with the other tools (boot.c): nothing
 * else names this file, so a constructor here would be dropped at the link.
 */
#include "openrac/host_env.h"
#include "rac1.h"

#define LEVELS 19
/* The boot program's game mode (6: the flight between planets) and the title loop's exit (a level is
 * on its way), as play.c reads them. */
#define BOOT_MODE RAC1_ADDR(0x0015F6E8u)
#define TITLE_EXIT RAC1_ADDR(0x0015F690u)
#define LEVEL_MODE OPENRAC_LDATA(0, RAC1_LADDR(00, 0x0015F6A8u))

#define HERO RAC1_ADDR(0x0013F450u)
#define HERO_MOBY (HERO + 0x2080u)
#define HERO_STATE (HERO + 0x2084u)
#define HERO_VEHICLE_MODE (HERO + 0x20A4u)

/* The hero's states by what the pad shows for them (HeroStateId). */
static int swimming(int state) {
    return state >= 0x33 && state <= 0x37; /* swim, swim idle, dive, water surface move, entry */
}

static int grinding(int state) {
    return (state >= 0x28 && state <= 0x2B) || state == 0x42;
}

static int on_hoverboard(int state) {
    return state == 0x3E || (state >= 0x6B && state <= 0x6F);
}

static const char* play_context(void) {
    const int vehicle = GREF(unsigned char, HERO_VEHICLE_MODE);
    const int state = GREF(int, HERO_STATE);
    if (vehicle == 1) {
        return "clank";
    }
    if (vehicle == 2) {
        return "giant_clank";
    }
    if (swimming(state)) {
        return "swim";
    }
    if (grinding(state)) {
        return "grind";
    }
    if (on_hoverboard(state)) {
        return "hoverboard";
    }
    return "foot";
}

int rac1_touch(const char** context, const char* hints[OPENRAC_TOUCH_HINTS_MAX]) {
    (void)hints;
    const int level = openrac_game_loaded_overlay();
    if (level < 0 || level >= LEVELS || !rac1_level_running()) {
        /* The title's menus; a level on its way (the flight between planets, a load). */
        const int on_way = (level >= 0 && level < LEVELS) || GREF(short, TITLE_EXIT) != 0
                           || GREF(int, BOOT_MODE) == 6;
        *context = on_way ? "cutscene" : "front_end";
        return 0;
    }
    switch (GREF(int, LEVEL_MODE)) {
        case 0:
            *context = GREF(gaddr, HERO_MOBY) != 0 ? play_context() : "cutscene";
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
