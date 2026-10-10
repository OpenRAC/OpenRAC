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
     * what a level's program moves is its data from 0x15F000 (resident below);
     * $gp is 0x166D00 in every program. */
    if (!relocation_set) {
        relocation_set = 1;
        openrac_guest_set_relocation(0x0015F000u, 0x00240000u, 0x00166D00u,
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
