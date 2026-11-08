/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Ratchet & Clank: the touch pad's context and hints (game_host.h, openrac_game_touch), read
 * from game memory once a frame. Addresses are games/rac1's (PAL), mapped per disc by
 * rac1_version.h; data the game keeps per level goes through OPENRAC_DATA, as hostgen's C does.
 *
 * Outside a level: "front_end", or "cutscene" while a level loads. In a level, by game mode: 0 play,
 * 3 pause ("menu", or "map" on the map page), 4 "dialog", 5 "vendor", else "cutscene". In play, by
 * the hero: Clank, Giant Clank, swimming, grinding, hoverboard, seated on a gun or in a jet.
 * Hints: a "press triangle" prompt (prompt()), the Hydro-Pack under water, the map's planet with an
 * infobot, the Quick Select page. Only reads. boot.c installs it (a constructor here would be dropped at the link).
 */
#include "openrac/host_env.h"
#include "rac1.h"

#define LEVELS 19
/* The boot program's game mode (6: the flight between planets) and the title loop's exit (a level is
 * on its way), as play.c reads them. */
#define BOOT_MODE RAC1_ADDR(0x0015F6E8u)
#define TITLE_EXIT RAC1_ADDR(0x0015F690u)
#define LEVEL_MODE OPENRAC_LDATA(0, RAC1_LADDR(00, 0x0015F6A8u))

/* The pause menu's page now (current_menu_page) and its map page (map_menu_page). The port moves
 * the game's data (OPENRAC_DATA), as the generated code reads it. */
#define MENU_PAGE OPENRAC_DATA(RAC1_ADDR(0x001D5F74u))
#define MAP_PAGE OPENRAC_DATA(RAC1_ADDR(0x001CF798u))
/* The Quick Select page, where L1 and R1 step through the slots (update_menu_item_slots). */
#define QUICK_PAGE OPENRAC_DATA(RAC1_ADDR(0x001CF538u))
/* The planet picked on the map (level_map_selection.level, map_state.h): O plays its infobot when it
 * is not 0 (draw_map_screen). */
#define MAP_LEVEL (OPENRAC_DATA(RAC1_ADDR(0x001A01F0u)) + 0x224u)

/* The items the hero has (item_available); 4 the Hydro-Pack, which R1 speeds up under water. */
#define ITEM_AVAILABLE RAC1_ADDR(0x0013D5C8u)
#define HYDRO_PACK 4u

#define HERO RAC1_ADDR(0x0013F450u)
#define HERO_MOBY (HERO + 0x2080u)
#define HERO_STATE (HERO + 0x2084u)
#define HERO_VEHICLE_MODE (HERO + 0x20A4u)

/* The help prompt up now (try_set_help_message, force_help_message): its owner (0 none; 6 a vehicle
 * or a guided shot hiding the prompts), its frames left and its message. */
#define HELP_KEEPALIVE OPENRAC_DATA(RAC1_ADDR(0x0015F6B0u))
#define HELP_OWNER OPENRAC_DATA(RAC1_ADDR(0x0015F6B4u))
#define HELP_ID OPENRAC_DATA(RAC1_ADDR(0x00161388u))

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

/* Seated (state 0x32, set only by these mobys): on a gun on Batalia, Gaspar and Hoven, in a jet on
 * Pokitaru, Gemlik and Drek's fleet. By the level: the hero's ship_moby is not cleared after a jet. */
static const char* seated(int level) {
    return level == 11 || level == 13 || level == 17 ? "jet" : "turret";
}

static const char* play_context(int level) {
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
    if (state == 0x32) {
        return seated(level);
    }
    return "foot";
}

/* A "press triangle" prompt's hint: the ship or a jet to board ("board"), a vendor ("buy"), or a lift,
 * a taxi, a teleporter, a giant Clank pad ("use"). Every prompt the game raises with a message reads
 * triangle (the mobys that call try_set_help_message), but for 0x53F1, a note on Orxon. */
static const char* prompt(void) {
    const int owner = GREF(int, HELP_OWNER);
    const int id = GREF(int, HELP_ID);
    if (owner == 0 || owner == 6 || id <= 0 || id == 0x53F1 || GREF(int, HELP_KEEPALIVE) <= 0) {
        return NULL;
    }
    if (id == 0x53E4 || owner == 7) {
        return "board";
    }
    if (id == 0x53E3 || (id >= 0x271E && id <= 0x2721)) {
        return "buy";
    }
    return "use";
}

int rac1_touch(const char** context, const char* hints[OPENRAC_TOUCH_HINTS_MAX]) {
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
            if (GREF(gaddr, HERO_MOBY) == 0) {
                *context = "cutscene";
                return 0;
            }
            *context = play_context(level);
            if (swimming(GREF(int, HERO_STATE))) {
                hints[0] = GREF(unsigned char, ITEM_AVAILABLE + HYDRO_PACK) != 0 ? "hydro" : NULL;
            } else {
                hints[0] = prompt();
            }
            return hints[0] != NULL ? 1 : 0;
        case 3:
            if (GREF(gaddr, MENU_PAGE) != MAP_PAGE) {
                *context = "menu";
                hints[0] = GREF(gaddr, MENU_PAGE) == QUICK_PAGE ? "slots" : NULL;
                return hints[0] != NULL ? 1 : 0;
            }
            *context = "map";
            if (GREF(int, MAP_LEVEL) != 0) {
                hints[0] = "infobot";
                return 1;
            }
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
