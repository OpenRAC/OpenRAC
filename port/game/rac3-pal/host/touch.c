/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal: the on-screen pad's context (game_host.h, openrac_game_touch), read from the
 * game's memory once a frame, as Going Commando's (rac2/host/touch.c). It only reads.
 *
 * Outside a story level (the boot program, frontbin, the multiplayer executable and its maps) the
 * menus ("front_end"). In a level by its mode word (tools.c, rac3_level_mode): 0 play, 4 the
 * overlays (the pause menus and the quick select's pause: "menu"), 5 a vendor ("vendor"), 8 a
 * weapon's upgrade ("dialog"), anything else a movie or a scene ("cutscene"). In play by the hero's
 * state (rac3-decomp enums/hero_state.h, Going Commando's numbering): swimming (the surface jump
 * 0x12, the underwater states 0x32 to 0x34, the surface swim 0x35), grinding (0x27 to 0x2A and
 * 0x3E), else on foot; with no health he is dying: "default", which has no strafe button, as
 * Going Commando's. No hints yet: the game's prompts are not read.
 * Installed with the tools (tools.c, rac3_install_tools).
 */
#include <stdint.h>

#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "rac3_version.h"

#define HERO_STATE RAC3_ADDR(0x001A71A4u)
#define HERO_HEALTH RAC3_ADDR(0x001A7430u)
#define STORY_LEVELS 37 /* 1 to 36 (vid-comics included); 39 on: the multiplayer menu and maps */

#define MODE_PLAY 0
#define MODE_OVERLAYS 4
#define MODE_VENDOR 5
#define MODE_UPGRADE 8

int rac3_level_mode(uint32_t* mode); /* tools.c */
int rac3_touch(const char** context, const char* hints[OPENRAC_TOUCH_HINTS_MAX]);

static int swimming(int state) {
    return state == 0x12 || (state >= 0x32 && state <= 0x35);
}

static int grinding(int state) {
    return (state >= 0x27 && state <= 0x2A) || state == 0x3E;
}

static const char* play_context(void) {
    const int state = GREF(int32_t, HERO_STATE);
    if (swimming(state)) {
        return "swim";
    }
    if (grinding(state)) {
        return "grind";
    }
    if (GREF(int32_t, HERO_HEALTH) <= 0) {
        return "default"; /* dying: no strafe button, so its toggle lets go */
    }
    return "foot";
}

int rac3_touch(const char** context, const char* hints[OPENRAC_TOUCH_HINTS_MAX]) {
    (void)hints;
    const int level = openrac_game_loaded_overlay();
    uint32_t mode = 0;
    if (level <= 0 || level >= STORY_LEVELS || !rac3_level_mode(&mode)) {
        *context = "front_end";
        return 0;
    }
    switch (mode) {
        case MODE_PLAY:
            *context = play_context();
            break;
        case MODE_OVERLAYS:
            *context = "menu";
            break;
        case MODE_VENDOR:
            *context = "vendor";
            break;
        case MODE_UPGRADE:
            *context = "dialog";
            break;
        default:
            *context = "cutscene";
            break;
    }
    return 0;
}
