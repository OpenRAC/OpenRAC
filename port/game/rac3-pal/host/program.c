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
#include "rac3_version.h"

#define FRONTEND_ENTRY RAC3_ADDR(0x00381250u)
#define FRONTBIN_OVERLAY 100
#define CURRENT_LEVEL                                                                              \
    RAC3_ADDR(0x001D545Cu) /* the level whose program is loaded, or is being loaded */
#define NEXT_LEVEL RAC3_ADDR(0x001D9D04u) /* the level a menu asks for; below zero, the front end */
#define FRONTEND_EXIT RAC3_ADDR(0x001D5B74u) /* the front end's loop ends when set */
#define LEVELS 64

gaddr func_0013AFF8__game(void);

static int loaded_overlay = OPENRAC_OVERLAY_EXE;

static int32_t word(gaddr at) {
    int32_t v;
    memcpy(&v, G(at), sizeof v);
    return v;
}

/* The programs' range (hostgen.json "relocate"): what a level's program replaces. */
#define PROGRAMS_LOW 0x001D5680u
#define PROGRAMS_HIGH 0x00700000u
#define PROGRAM_BUFFER                                                                             \
    RAC3_ADDR(0x001D4B50u)     /* where the next program was read: a header word, its records */
#define PROGRAM_GP 0x001DC8B0u /* $gp, crt0's, the same in every program (hostgen.json "gp") */

/* A level function the catalogue names after another level (func_L05_...) is written with that
 * level's addresses; the runtime pairs them with the loaded level's by their code (guest.h,
 * OPENRAC_LDATA), from each level's program as the game reads it. Kept as it is read, before
 * 0x0013AFF8 copies it over the boot program. */
static void keep_level_program(int level) {
    static uint64_t kept;
    if (level < 0 || level >= LEVELS || (kept >> level) & 1u) {
        return;
    }
    const gaddr buffer = (gaddr)word(PROGRAM_BUFFER);
    gaddr at = buffer + (gaddr)word(buffer);
    const gaddr entry = (gaddr)word(at + 12u);
    while (at + 16u <= 0x02000000u && (gaddr)word(at + 12u) == entry) {
        const gaddr base = (gaddr)word(at), size = (gaddr)word(at + 4u);
        if (size > 0x02000000u - (at + 16u)) {
            break;
        }
        openrac_guest_add_level_program(level, base, (const uint8_t*)G(at + 16u), size);
        at += 16u + size;
    }
    kept |= (uint64_t)1 << level;
}

gaddr func_0013AFF8(void) {
    static int range_set;
    if (!range_set) {
        range_set = 1;
        /* Only $gp: a level's small globals are reached from it (the same in every program), and
         * the pairs of a level function's copies need it to pair them (Florana's copy of
         * func_L01_004E52E0 forms its joint table as $gp - 0xA08, Veldin's as $gp - 0x8C8; without
         * it the table stayed Veldin's address and the hero's joints were read from garbage). No
         * range: the boot program's own C is not relocated (its front end gives way to frontbin's
         * and the levels' copies, which hostgen names apart). */
        openrac_guest_set_relocation(0, 0, PROGRAM_GP, NULL, 0, 0);
        openrac_guest_set_level_range(PROGRAMS_LOW, PROGRAMS_HIGH);
    }
    const int level = word(CURRENT_LEVEL);
    if (level >= 0 && level < LEVELS) {
        keep_level_program(level);
    }
    const gaddr entry = func_0013AFF8__game();
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

/* request_level_change (frontbin's 0x00392DF0, NTSC-U 0x00417DA0): (level, save) sets the level's
 * level_next and its change request, which the level's main loop (Veldin's func_L01_003F08C8) acts
 * on at the top of its next frame: level_change tears the level down and level_switch loads the
 * next one with the transition level_load picks for the pair. Each story level's program has its
 * own copy (rac3-decomp config/levels/functions.tsv, func_00392DF0's places); the multiplayer maps
 * have none. */
static const struct {
    int8_t level;
    gaddr at;
} kRequestLevelChange[] = {
    {1, 0x0041A3F0u},  {2, 0x003F3048u},  {3, 0x004078A0u},  {4, 0x0042B178u},  {5, 0x004141D8u},
    {6, 0x003FEEB0u},  {7, 0x0041AC90u},  {8, 0x003FDC10u},  {9, 0x00433840u},  {10, 0x003FDAC0u},
    {11, 0x003F3D38u}, {12, 0x0041BE88u}, {13, 0x003FBAC0u}, {14, 0x00404FB0u}, {16, 0x0042A090u},
    {17, 0x003F5BD0u}, {18, 0x0042D8C8u}, {19, 0x0040F848u}, {20, 0x00438D70u}, {21, 0x003F2FC0u},
    {22, 0x0041FB90u}, {23, 0x003FD228u}, {24, 0x004026B8u}, {26, 0x004244C0u}, {27, 0x003F3A68u},
    {28, 0x003E6608u}, {29, 0x00426DC8u}, {30, 0x003EDEA8u}, {31, 0x003F2268u}, {32, 0x003FAAE8u},
    {33, 0x003F2EA0u}, {34, 0x003F4610u}, {35, 0x003F3610u}, {36, 0x003F9028u},
};

/* What a menu's choice writes (0x003B3A80: the exit word and the level to go to), so the front end
 * ends and loads that program itself; a level below zero is frontbin. */
static void front_end_go(int level) {
    const int32_t exit_word = 1, next = (int32_t)level;
    memcpy(G(NEXT_LEVEL), &next, sizeof next);
    memcpy(G(FRONTEND_EXIT), &exit_word, sizeof exit_word);
}

/* --level N with OPENRAC_DIRECT, OPENRAC_RAC3_NEXT and the developer menu: from the front end (the
 * boot program or frontbin) as its menus leave for a level; from a level through its own
 * request_level_change, as a level's exit asks for the next. Returns 0 when the level loaded has no
 * way out by it (the multiplayer maps). */
int rac3_start_level(int level) {
    if (level < 0 || level >= LEVELS) {
        return 0;
    }
    if (loaded_overlay == OPENRAC_OVERLAY_EXE || loaded_overlay == FRONTBIN_OVERLAY) {
        front_end_go(level);
        fprintf(stderr, "[info] rac3: the front end asked for level %d\n", level);
        return 1;
    }
    for (size_t i = 0; i < sizeof kRequestLevelChange / sizeof kRequestLevelChange[0]; ++i) {
        if (kRequestLevelChange[i].level == loaded_overlay) {
            const uint64_t ints[8] = {(uint64_t)(int64_t)level, 0};
            const float floats[8] = {0};
            float result = 0;
            openrac_guest_call_regs(kRequestLevelChange[i].at, ints, floats, &result, 0);
            fprintf(stderr, "[info] rac3: level %d asked for level %d\n", loaded_overlay, level);
            return 1;
        }
    }
    return 0;
}

/* OPENRAC_RAC3_NEXT=FRAME:LEVEL, and --level N with OPENRAC_DIRECT (at the 60th list): at the boot
 * program's FRAME-th display list, the front end is sent to LEVEL (a LEVEL below zero is frontbin).
 * Called with each display list sent (dma.c). */
#define DIRECT_FRAME 60

void rac3_program_probe(void) {
    static long frame = -1, level;
    static long sent;
    if (frame < 0) {
        const char* v = openrac_env_string("OPENRAC_RAC3_NEXT");
        char* end = NULL;
        frame = v != NULL ? strtol(v, &end, 10) : 0;
        level = end != NULL && *end == ':' ? strtol(end + 1, NULL, 10) : -1;
        if (frame <= 0 && openrac_game_start_level >= 0
            && openrac_env_string("OPENRAC_DIRECT") != NULL) {
            frame = DIRECT_FRAME;
            level = openrac_game_start_level;
        }
    }
    if (frame <= 0 || loaded_overlay != OPENRAC_OVERLAY_EXE || ++sent != frame) {
        return;
    }
    front_end_go((int)level);
    fprintf(stderr, "[info] rac3: the front end was sent to level %ld\n", level);
}
