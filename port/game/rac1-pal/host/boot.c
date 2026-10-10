/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Around the game's own start-up code (libraries.tsv, port = wrap). */
#include "game_protos.h"
#include "openrac/game_host.h"

/* The level being played, or -1 (docs/port/RAC1_PAL_SURVEY.md). */
#define CURRENT_LEVEL 0x0015EE84u
#define LEVELS 19

static int loaded_level = OPENRAC_OVERLAY_EXE;

/* ParseBin: copies the next level's program over the executable's main
 * segment and returns its entry. From then on a code address in that range
 * means the loaded level's function (the overlay the runtime looks in). */
gaddr func_0012DA38(void) {
    static int relocation_set;
    /* Before the first level is copied in: the executable's code as the
     * pattern its level copies are paired against (guest.h, per-level
     * relocation). The executable's code and data are 0x100000-0x240000;
     * what a level's program moves is its data from 0x15F000 (resident below)
     * and the end-of-program address InitMemSlots plans memory from
     * (0x24272F: the end rounded up, which a level's copy has as its own);
     * $gp is 0x166D00 in every program. */
    if (!relocation_set) {
        relocation_set = 1;
        openrac_guest_set_relocation(0x0015F000u, 0x00250000u, 0x00166D00u,
                                     (const uint8_t *)G(0x00100000u), 0x00100000u, 0x00140000u);
    }
    const gaddr entry = func_0012DA38__game();
    loaded_level = GREF(int, CURRENT_LEVEL);
    return entry;
}

/* hostgen.json: "overlay_hook": true. */
int openrac_game_loaded_overlay(void) {
    return loaded_level >= 0 && loaded_level < LEVELS ? loaded_level : OPENRAC_OVERLAY_EXE;
}

/* SetupGifPaging, every frame before the texture paging. The four words at 0x18A3D0 are flags the
 * draw code tests (shrubs at +0, mobys at +8, the effect-texture uploads at +0xC: without that one
 * the font and the title's logo are never uploaded). Retail has all four set to 1 by its first
 * frame; no code in the game stores them by name, so they are set here, once for each program
 * (the executable, then each level, whose copies are elsewhere: OPENRAC_DATA). */
void func_001F4630(int arg0) {
    static int set_for = -2;
    if (set_for != loaded_level) {
        set_for = loaded_level;
        for (unsigned i = 0; i < 4; ++i) {
            GREF(int, OPENRAC_DATA(0x0018A3D0u + i * 4)) = 1;
        }
    }
    func_001F4630__game(arg0);
}

/* NewGameInit: a new game's state is a fresh save restored from the disc, with the level set to 0
 * (Veldin); the level's files are read right after. With --level N the new game starts in level N
 * instead, the way the behaviour checker reaches a level with its level write. */
void func_00209DC0(void) {
    func_00209DC0__game();
    if (openrac_game_start_level >= 0 && openrac_game_start_level < LEVELS) {
        GREF(int, CURRENT_LEVEL) = openrac_game_start_level;
    }
}
