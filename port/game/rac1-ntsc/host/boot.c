/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Around the game's own start-up code (libraries.tsv, port = wrap): the
 * NTSC-U counterpart of rac1-pal's host/boot.c. */
#include "game_protos.h"
#include "openrac/game_host.h"

/* The level being played, or -1: NTSC-U 0x15ED84 (PAL 0x15EE84; ReRAC's
 * docs/plan/game_state.md, the game-state block). */
#define CURRENT_LEVEL 0x0015ED84u
#define LEVELS 19

static int loaded_level = OPENRAC_OVERLAY_EXE;

/* parse_bin (0x12D8F8): copies the next level's program over the
 * executable's main segment and returns its entry, which main
 * (run_game_main_loop, 0x12D9D8) calls next. From then on a code address in
 * that range means the loaded level's function (the overlay the runtime
 * looks in). */
long long game_FUN_0012d8f8(void) {
    const long long entry = game_FUN_0012d8f8__game();
    loaded_level = GREF(int, CURRENT_LEVEL);
    return entry;
}

/* hostgen.json: "overlay_hook": true. */
int openrac_game_loaded_overlay(void) {
    return loaded_level >= 0 && loaded_level < LEVELS ? loaded_level : OPENRAC_OVERLAY_EXE;
}
