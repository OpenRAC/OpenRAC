/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal: which program is loaded (libraries.tsv, port = wrap; Going Commando's is
 * rac2/host/level.c). main (0x0013B330) runs a program's entry; when it returns, 0x0013AFF8 copies
 * the next program over the boot program's (its records, from .lit at 0x001D5680 up) and returns
 * its entry, and the loop goes round.
 *
 * The boot program's front end (0x00381250) ends when a menu sets the exit word (0x001D5B74) and
 * the level to go to (0x001D9D04); on its way out (0x003A7CB8, 0x003BD870) it reads frontbin, the
 * front end's own program in the MISC wad, when that level is below zero (0x003BD160, 0x0039A530),
 * else the level's program, whose number it keeps in 0x001D545C. Both are read into a buffer at
 * the top of memory (*0x001D4B50) that 0x0013AFF8 copies from. frontbin's entry is the front end's
 * own address, 0x00381250; a level's is its own.
 *
 * The code addresses of the program loaded are that program's functions: hostgen's overlay 100
 * for frontbin (hostgen.json "groups"), the level's number for a level. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "openrac/host_env.h"

#define FRONTEND_ENTRY 0x00381250u
#define FRONTBIN_OVERLAY 100
#define CURRENT_LEVEL 0x001D545Cu /* the level whose program is loaded, or is being loaded */
#define NEXT_LEVEL 0x001D9D04u    /* the level a menu asks for; below zero, the front end */
#define FRONTEND_EXIT 0x001D5B74u /* the front end's loop ends when set */
#define LEVELS 64

gaddr func_0013AFF8__game(void);

static int loaded_overlay = OPENRAC_OVERLAY_EXE;

static int32_t word(gaddr at) {
    int32_t v;
    memcpy(&v, G(at), sizeof v);
    return v;
}

gaddr func_0013AFF8(void) {
    const gaddr entry = func_0013AFF8__game();
    const int level = word(CURRENT_LEVEL);
    if (entry == FRONTEND_ENTRY) {
        loaded_overlay = FRONTBIN_OVERLAY;
    } else if (level >= 0 && level < LEVELS) {
        loaded_overlay = level;
    } else {
        loaded_overlay = OPENRAC_OVERLAY_EXE;
    }
    fprintf(
        stderr,
        "[info] rac3: next program: %s %d, entry 0x%08x\n",
        loaded_overlay == FRONTBIN_OVERLAY ? "frontbin, overlay" : "level",
        loaded_overlay == FRONTBIN_OVERLAY ? FRONTBIN_OVERLAY : level,
        entry
    );
    return entry;
}

/* hostgen.json: "overlay_hook": true. */
int openrac_game_loaded_overlay(void) {
    return loaded_overlay;
}

/* A warm save-state restores the program and the runtime's overlay together. */
void openrac_game_restore_overlay(int overlay) {
    loaded_overlay = overlay == FRONTBIN_OVERLAY || (overlay >= 0 && overlay < LEVELS)
                         ? overlay
                         : OPENRAC_OVERLAY_EXE;
}

/* OPENRAC_RAC3_NEXT=FRAME:LEVEL, a development probe until the menus can be read: at the boot
 * program's FRAME-th display list, what a menu's choice writes (0x003B3A80: the exit word and the
 * level to go to), so the front end ends and loads that program itself; a LEVEL below zero is
 * frontbin. Called with each display list sent (dma.c). */
void rac3_program_probe(void) {
    static long frame = -1, level;
    static long sent;
    if (frame < 0) {
        const char* v = openrac_env_string("OPENRAC_RAC3_NEXT");
        char* end = NULL;
        frame = v != NULL ? strtol(v, &end, 10) : 0;
        level = end != NULL && *end == ':' ? strtol(end + 1, NULL, 10) : -1;
    }
    if (frame <= 0 || loaded_overlay != OPENRAC_OVERLAY_EXE || ++sent != frame) {
        return;
    }
    const int32_t exit_word = 1, next = (int32_t)level;
    memcpy(G(NEXT_LEVEL), &next, sizeof next);
    memcpy(G(FRONTEND_EXIT), &exit_word, sizeof exit_word);
    fprintf(stderr, "[info] rac3: OPENRAC_RAC3_NEXT: the front end asked for level %d\n", next);
}
