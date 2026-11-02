/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Going Commando: the view tests of the draw callbacks in a picture wider than the game's, as Ratchet
 * & Clank's (rac1/host/wide_view.c says what they are and why). The camera block (BuildViewMatrices,
 * 0x001B5200) and the view block (UpdateViewContext, 0x001B9040) have the same layout: the sphere test
 * func_00283DB8 reads the clip constants at the view block's +0x2C0. The game keeps five draw callback
 * lists, one more than Ratchet & Clank, each run by its own routine. Only the lists' runs are opened:
 * nothing here maps the levels' own updates that test what the player sees yet (rac1's orbs, traffic
 * and fires).
 */
#include "game_protos.h"

#define WIDE_CAMERA OPENRAC_DATA(0x001B5200u)
#define WIDE_VIEW OPENRAC_DATA(0x001B9040u)
#include "../../common/lib/wide_view_lists.h"

static const unsigned char* g_fix_liquids;

static void run_list(void (*run)(void)) {
    wide_run_list(run, wide_wider_for(&g_fix_liquids, "fix_wide_liquids"));
}

void func_0027CE18(void) {
    run_list(func_0027CE18__game);
}

void func_0027CEE0(void) {
    run_list(func_0027CEE0__game);
}

void func_0027CF58(void) {
    run_list(func_0027CF58__game);
}

void func_0027D020(void) {
    run_list(func_0027D020__game);
}

void func_0027D0E8(void) {
    run_list(func_0027D0E8__game);
}

/* FastBSphereCheck: 1 in view, 0 in view but across the guard band's edge, -1 culled. */
int func_00283DB8(gaddr sphere, float far) {
    return wide_sphere(func_00283DB8__game, sphere, far, 0, 1.0f);
}
