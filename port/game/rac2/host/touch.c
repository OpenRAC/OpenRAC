/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Going Commando: the on-screen pad's context and hints (game_host.h, openrac_game_touch), read
 * from the game's memory once a frame. It only reads.
 *
 * Outside a level the title and its menus ("front_end"). In a level by its game state
 * (g_nGameState, as rac1's game mode): 0 play, 3 the pause menu ("menu", or "map" on the Galactic
 * Map), 4 a dialog ("dialog"; quick select keeps play's), 5 a vendor ("vendor"), anything else a
 * movie, a scene or a load ("cutscene"). In play the space combat levels are the ship's, Giant
 * Clank his own, then swimming, grinding and on foot. Hints: a "press triangle" prompt.
 * Installed with the tools (tools.c, rac2_install_tools).
 */
#include "openrac/game_host.h"
#include "openrac/host_env.h"
#include "rac2.h"

#define LEVELS 31
/* Feltzin System, Hrugis Cloud, Gorn, Wupash: the space combat levels. */
#define LEVEL_FELTZIN 5
#define LEVEL_HRUGIS 10
#define LEVEL_GORN 15
#define LEVEL_WUPASH 25

/* The hero's vehicle byte (as rac1's hero +0x20A4): 2 is Giant Clank. */
#define HERO_VEHICLE_MODE (RAC2_g_heroState + 0x20u)

/* Words the other releases have only once release_carry.py has carried them (RAC2_ADDR_...);
 * until then they show the pause and dialog sets there, and no prompts. */
#if defined(RAC2_PRIMARY) || defined(RAC2_ADDR_0x001AA0F8u)
#define STATE_ARG OPENRAC_DATA(RAC2_ADDR(0x001AA0F8u)) /* g_gameStatePendingArgA */
#endif
#if defined(RAC2_PRIMARY) || defined(RAC2_ADDR_0x001A8C64u)
/* The HUD prompt (func_002B0C28): its owner, frames left and text. */
#define PROMPT_OWNER OPENRAC_DATA(RAC2_ADDR(0x001A8C64u)) /* g_noticeOwner */
#define PROMPT_TIMER OPENRAC_DATA(RAC2_ADDR(0x001A8C60u))
#define PROMPT_TEXT OPENRAC_DATA(RAC2_ADDR(0x001B1D48u))
#endif
/* Under state 3 the Galactic Map, under 4 quick select (hold triangle). */
#define ARG_MAP 2
#define ARG_QUICK 7

static int swimming(int state) {
    return state >= 0x33 && state <= 0x37;
}

static int grinding(int state) {
    return (state >= 0x28 && state <= 0x2B) || state == 0x42;
}

static const char* play_context(int level) {
    if (level == LEVEL_FELTZIN || level == LEVEL_HRUGIS || level == LEVEL_GORN
        || level == LEVEL_WUPASH) {
        return "ship";
    }
    if (GREF(uint32_t, RAC2_g_pHeroMoby) == 0) {
        return "cutscene";
    }
    const int state = GREF(int32_t, RAC2_g_heroState);
    if (GREF(uint8_t, HERO_VEHICLE_MODE) == 2) {
        return "giant_clank";
    }
    if (swimming(state)) {
        return "swim";
    }
    if (grinding(state)) {
        return "grind";
    }
    return "foot";
}

/* A "press triangle" prompt's hint, by who raised it: 1 a vendor or the Ship Shack ("buy"), 2 and
 * 7 the ship, or "Return to ship" ("board"), 5 a help popup and 6 a hidden prompt (none), the rest
 * an elevator, a race, a translocator and the like ("use"). */
static const char* prompt(void) {
#ifdef PROMPT_OWNER
    const int owner = GREF(int32_t, PROMPT_OWNER);
    const int text = GREF(int32_t, PROMPT_TEXT);
    if (owner == 0 || owner == 5 || owner == 6 || text <= 0 || GREF(int32_t, PROMPT_TIMER) <= 0) {
        return NULL;
    }
    if (owner == 1) {
        return "buy";
    }
    if (owner == 2 || owner == 7 || text == 0x2793) {
        return "board";
    }
    return "use";
#else
    return NULL;
#endif
}

static int state_arg(void) {
#ifdef STATE_ARG
    return GREF(int32_t, STATE_ARG);
#else
    return 0;
#endif
}

int rac2_touch(const char** context, const char* hints[OPENRAC_TOUCH_HINTS_MAX]) {
    const int level = openrac_game_loaded_overlay();
    if (level < 0 || level >= LEVELS) {
        *context = "front_end";
        return 0;
    }
    switch (GREF(int32_t, OPENRAC_DATA(RAC2_g_nGameState))) {
        case 0:
            *context = play_context(level);
            hints[0] = (*context)[0] == 'f' ? prompt() : NULL; /* "foot": the sets with the prompts */
            return hints[0] != NULL ? 1 : 0;
        case 3:
            *context = state_arg() == ARG_MAP ? "map" : "menu";
            return 0;
        case 4:
            *context = state_arg() == ARG_QUICK ? play_context(level) : "dialog";
            return 0;
        case 5:
            *context = "vendor";
            return 0;
        default:
            *context = "cutscene";
            return 0;
    }
}
