/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Going Commando: which program is loaded (libraries.tsv, port = wrap; rac1's is rac1/host/boot.c).
 * main (0x00131D98) runs a program's loop, then ParseBin installs the next program over the
 * executable and returns its entry, and the loop goes round. */
#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "rac2.h"

/* The executable's own loop: the title and the menus. */
#define TITLE_LOOP RAC2_ADDR(0x0026EDE8u)

/* The save's block, below the programs (g_playerProgress): its first word is the level. */
#define PLAYER_PROGRESS RAC2_ADDR(0x001A79F0u)

/* What a level's program replaces: hostgen.json's "relocate". $gp is the same in every program. */
#define RELOCATE_LOW RAC2_LEVEL_BASE
#define RELOCATE_HIGH 0x00500000u
#define GP RAC2_GP

/* The executable as loaded, whose code a level's copies are paired against: the ELF's one code
 * segment, 0x100080 to 0x351FC0. */
#define EXE_BASE 0x00100000u
#define EXE_SIZE RAC2_EXE_SIZE

/* The levels, by the game's own numbers: 0 to 30, some unused (games/rac2/decomp/docs/LEVELS.md). */
#define LEVELS 31
#define MUSEUM 21
#define MUSEUM_PROGRAM 30

gaddr func_00131CB8__game(void);

static int loaded_level = OPENRAC_OVERLAY_EXE;

/* ParseBin: copies the next program in. From then on a code address in the range means the loaded
 * level's function (the overlay the runtime looks in). */
gaddr func_00131CB8(void) {
    static int relocation_set;
    if (!relocation_set) {
        relocation_set = 1;
        openrac_guest_set_relocation(
            RELOCATE_LOW, RELOCATE_HIGH, GP, (const uint8_t*)G(EXE_BASE), EXE_BASE, EXE_SIZE
        );
        openrac_guest_set_level_range(RELOCATE_LOW, RELOCATE_HIGH);
    }
    const gaddr entry = func_00131CB8__game();
    const int level = GREF(int, PLAYER_PROGRESS);
    if (entry == TITLE_LOOP || level < 0 || level >= LEVELS) {
        /* Back to the title: the port runs the executable's C, so its own program is what has to be
         * in memory, as on rac1 (openrac_guest_restore_executable). The copy on the disc Quit Game
         * reads is another build of it, its loop at 0x0026AD20, so the entry is the executable's. */
        loaded_level = OPENRAC_OVERLAY_EXE;
        openrac_guest_restore_executable(RELOCATE_LOW, RELOCATE_HIGH);
        return TITLE_LOOP;
    } else {
        /* The Insomniac Museum is level 21 to the game and to the disc's table, and its program, the
         * catalogue's and the export's numbers are the 30 its archive says. */
        loaded_level = level == MUSEUM ? MUSEUM_PROGRAM : level;
    }
    return entry;
}

/* hostgen.json: "overlay_hook": true. */
int openrac_game_loaded_overlay(void) {
    return loaded_level;
}

/* A warm save-state restores the level and the runtime's overlay together. */
void openrac_game_restore_overlay(int overlay) {
    loaded_level = overlay >= 0 && overlay < LEVELS ? overlay : OPENRAC_OVERLAY_EXE;
}
