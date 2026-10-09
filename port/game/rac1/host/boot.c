/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Around the game's own start-up code (libraries.tsv, port = wrap). */
#include "game_protos.h"
#include "rac1_host.h"

/* The level being played, or -1 (docs/port/RAC1_PAL_SURVEY.md). */
#define CURRENT_LEVEL 0x0015EE84u

static int loaded_level = OPENRAC_OVERLAY_EXE;

/* ParseBin: copies the next level's program over the executable's main
 * segment and returns its entry. From then on a code address in that range
 * means the loaded level's function (the overlay the runtime looks in). */
gaddr func_0012DA38(void) {
    const gaddr entry = func_0012DA38__game();
    loaded_level = GREF(int, CURRENT_LEVEL);
    return entry;
}

int openrac_rac1_loaded_level(void) {
    return loaded_level;
}
