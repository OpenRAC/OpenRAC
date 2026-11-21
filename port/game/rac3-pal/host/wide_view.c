/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Up Your Arsenal: the view tests of the draw callbacks in a picture wider than the game's, as Going
 * Commando's (rac2/host/wide_view.c) and Ratchet & Clank's (rac1/host/wide_view.c says what they are
 * and why). The blocks have Going Commando's layout:
 *
 * - the camera block (Veldin's 0x001E2880, 0x160 before BuildViewMatrices' rows, its eye at +0x140 as
 *   MobyProc reads it): +0x100 the world-to-clip matrix that culls;
 * - the view context (Veldin's 0x001E7580, UpdateViewContext's): the sphere test reads its +0x280
 *   matrix and the clip constants at +0x2C0 (the tangents), +0x2E0 (the secants), as Going
 *   Commando's FastBSphereCheck (func_00283DB8) does, the same instructions as this game's
 *   (frontbin's func_0038DC20, Veldin's place func_L01_0040F5F0, 49 levels).
 *
 * DrawFrame (Veldin's 0x00403440) runs five draw callback lists, each by its own runner, the same
 * routines as Going Commando's five (func_0027CE18, func_0027D020, func_0027CEE0, func_0027CF58,
 * func_0027D0E8; the sizes line up one for one): a list is a count and tables of functions and
 * arguments 0x100 bytes apart (the fifth's 0x10 apart, of four). Each list's run is opened as wide as
 * the picture (game fix fix_wide_liquids), and each callback's argument is kept as the owner of the
 * world quads it draws (render.c: a moby's effects move with its body between ticks).
 *
 * Written with Veldin's copies, as render.c; the multiplayer maps' (levels 39 to 53) are not wrapped.
 */
#include <stdint.h>

#include "game_protos.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "rac3_version.h"

/* A global of Veldin's program, in the program loaded now. */
#define VELDIN(a) OPENRAC_LDATA(1, (gaddr)(a))

#define WIDE_CAMERA VELDIN(RAC3_LADDR(1, 0x001E2880u))
#define WIDE_VIEW VELDIN(RAC3_LADDR(1, 0x001E7580u))
#include "../../common/lib/wide_view_lists.h"

/* The running draw callback's argument and function, 0 outside them, and how many world quads it
 * has drawn so far (render.c: the quads' owner and ids). */
gaddr rac3_draw_owner;
gaddr rac3_draw_callback;
unsigned rac3_draw_ordinal;

static const unsigned char* g_fix_liquids;

static void run_list(void (*run)(void)) {
    float taller;
    const float wider = wide_wider_for(&g_fix_liquids, "fix_wide_liquids", &taller);
    wide_run_list(run, wider, taller);
}

/* The count and the two tables a list's runner reads, from the runner's own instructions in the
 * loaded level: each level's program has its copy of the runner and of its tables at its own
 * addresses, which the runner forms with lui and %lo (lw for the count, addiu for each table). In
 * every runner they come in the same order: the count, the arguments, the functions. Relocating
 * Veldin's addresses through the data pairs put Obani Moons' tables 0x100 off: it called its
 * arguments, mobys, as functions. 0 when the code does not have that shape. */
static int runner_tables(gaddr veldin_runner, gaddr* count_at, gaddr* functions, gaddr* arguments) {
    const gaddr code = OPENRAC_LCODE(1, veldin_runner);
    uint32_t hi[32] = {0};
    gaddr found[3];
    int n = 0;
    for (int i = 0; i < 30 && n < 3; ++i) {
        const uint32_t w = GREF(uint32_t, code + 4u * (gaddr)i);
        const unsigned op = w >> 26, rs = (w >> 21) & 31u, rt = (w >> 16) & 31u;
        const int32_t lo = (int16_t)(w & 0xFFFFu);
        if (op == 0x0Fu) { /* lui */
            hi[rt] = w << 16;
            continue;
        }
        if ((op == 0x09u || op == 0x23u) && hi[rs] != 0) { /* addiu, lw */
            const gaddr at = hi[rs] + (gaddr)lo;
            int seen = 0;
            for (int k = 0; k < n; ++k) {
                seen |= found[k] == at;
            }
            if (!seen) {
                found[n++] = at;
            }
        }
    }
    if (n != 3) {
        return 0;
    }
    *count_at = found[0];
    *arguments = found[1];
    *functions = found[2];
    return 1;
}

/* A list's run, as the game's runners (the count read again before each call, as a callback may add
 * one), each callback's argument kept as the owner of what it draws. */
static void run_callbacks(gaddr veldin_runner) {
    gaddr count_at, functions, arguments;
    if (!runner_tables(veldin_runner, &count_at, &functions, &arguments)) {
        return;
    }
    const gaddr outer = rac3_draw_owner, outer_callback = rac3_draw_callback;
    const unsigned outer_ordinal = rac3_draw_ordinal;
    for (int index = 0; index < GREF(int, count_at); ++index) {
        const gaddr argument = GREF(gaddr, arguments + (gaddr)index * 4u);
        const gaddr function = GREF(gaddr, functions + (gaddr)index * 4u);
        rac3_draw_owner = argument;
        rac3_draw_callback = function;
        rac3_draw_ordinal = 0;
        GFN(void (*)(int), function)((int)argument);
    }
    rac3_draw_owner = outer;
    rac3_draw_callback = outer_callback;
    rac3_draw_ordinal = outer_ordinal;
}

static void run_first(void) {
    run_callbacks(RAC3_LADDR(1, 0x00404858u));
}

static void run_second(void) {
    run_callbacks(RAC3_LADDR(1, 0x00404A60u));
}

static void run_third(void) {
    run_callbacks(RAC3_LADDR(1, 0x00404920u));
}

static void run_fourth(void) {
    run_callbacks(RAC3_LADDR(1, 0x00404998u));
}

static void run_fifth(void) {
    run_callbacks(RAC3_LADDR(1, 0x00404B28u));
}

void func_L01_00404858(void) {
    run_list(run_first);
}

void func_L01_00404A60(void) {
    run_list(run_second);
}

void func_L01_00404920(void) {
    run_list(run_third);
}

void func_L01_00404998(void) {
    run_list(run_fourth);
}

void func_L01_00404B28(void) {
    run_list(run_fifth);
}

/* The sphere test: 1 in view, 0 across the guard band's edge, -1 culled; opened while a list runs. */
static int game_sphere(gaddr sphere, float far) {
    return func_L01_0040F5F0__game((int)sphere, far);
}

int func_L01_0040F5F0(int sphere, float far) {
    return wide_sphere(game_sphere, (gaddr)sphere, far, 0, 1.0f, 1.0f);
}
