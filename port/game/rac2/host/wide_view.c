/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Going Commando: the view tests of the draw callbacks in a picture wider than the game's, as Ratchet
 * & Clank's (rac1/host/wide_view.c says what they are and why). The camera block (BuildViewMatrices,
 * 0x001B5200) and the view block (UpdateViewContext, 0x001B9040) have the same layout: the sphere test
 * func_00283DB8 reads the clip constants at the view block's +0x2C0. The game keeps five draw callback
 * lists, one more than Ratchet & Clank, each run by its own routine. The lists' runs are opened, and
 * of the levels' own updates that test what the player sees, the nanotech cluster's (rac1's orbs).
 * Ratchet & Clank's other two have no pair here: its air traffic is Kerwan's, and its smoke and fire
 * emitter pairs with nothing above 0.09 (RAC1_MAP); 85 functions call the sphere test, not all read.
 */
#include "draw_owner.h"
#include "game_protos.h"

#define WIDE_CAMERA OPENRAC_DATA(RAC2_ADDR(0x001B5200u))
#define WIDE_VIEW OPENRAC_DATA(RAC2_ADDR(0x001B9040u))
#include "rac2.h"
#include "../../common/lib/wide_view_lists.h"

static const unsigned char* g_fix_liquids;
static const unsigned char* g_fix_orbs;

/* How many updates whose sphere test stands for what the player sees are running (the nanotech
 * cluster's), and how much wider the picture is while they do. */
static int g_effects;
static float g_effect_wider = 1.0f;
static float g_effect_taller = 1.0f;

static void run_list(void (*run)(void)) {
    float taller;
    const float wider = wide_wider_for(&g_fix_liquids, "fix_wide_liquids", &taller);
    wide_run_list(run, wider, taller);
}

/* A draw callback list's run, as the game's runners (src/game/0027B948.c: the count read again before
 * each call, as a callback may add one), with each callback's argument kept as the owner of the world
 * quads it draws (render.c, draw_owner.h): most are a moby's own effects, given the moby. */
static void run_callbacks(gaddr count_at, gaddr functions, gaddr arguments) {
    const gaddr outer = rac2_draw_owner;
    for (int index = 0; index < GREF(int, count_at); ++index) {
        const gaddr argument = GREF(gaddr, arguments + (gaddr)index * 4u);
        rac2_draw_owner = argument;
        GFN(void (*)(int), GREF(gaddr, functions + (gaddr)index * 4u))((int)argument);
    }
    rac2_draw_owner = outer;
}

static void run_first(void) {
    run_callbacks(OPENRAC_DATA(RAC2_ADDR(0x001B1608u)), OPENRAC_DATA(RAC2_ADDR(0x001B9E40u)), OPENRAC_DATA(RAC2_ADDR(0x001B9F40u)));
}

static void run_second(void) {
    run_callbacks(OPENRAC_DATA(RAC2_ADDR(0x001B160Cu)), OPENRAC_DATA(RAC2_ADDR(0x001BA040u)), OPENRAC_DATA(RAC2_ADDR(0x001BA140u)));
}

static void run_third(void) {
    run_callbacks(OPENRAC_DATA(RAC2_ADDR(0x001B1610u)), OPENRAC_DATA(RAC2_ADDR(0x001BA240u)), OPENRAC_DATA(RAC2_ADDR(0x001BA340u)));
}

static void run_fourth(void) {
    run_callbacks(OPENRAC_DATA(RAC2_ADDR(0x001B1614u)), OPENRAC_DATA(RAC2_ADDR(0x001BA440u)), OPENRAC_DATA(RAC2_ADDR(0x001BA540u)));
}

static void run_fifth(void) {
    run_callbacks(OPENRAC_DATA(RAC2_ADDR(0x001B1638u)), OPENRAC_DATA(RAC2_ADDR(0x001B1618u)), OPENRAC_DATA(RAC2_ADDR(0x001B1628u)));
}

void func_0027CE18(void) {
    run_list(run_first);
}

void func_0027CEE0(void) {
    run_list(run_third);
}

void func_0027CF58(void) {
    run_list(run_fourth);
}

void func_0027D020(void) {
    run_list(run_second);
}

void func_0027D0E8(void) {
    run_list(run_fifth);
}

/* FastBSphereCheck: 1 in view, 0 in view but across the guard band's edge, -1 culled. */
int func_00283DB8(gaddr sphere, float far) {
    return wide_sphere(
        func_00283DB8__game, sphere, far, g_effects, g_effect_wider, g_effect_taller
    );
}

/* The nanotech cluster's update (the orb on a nanotech crate; Ratchet & Clank's
 * NanotechBallUpdate, paired at 0.47 and read in the functional C): it sends its draw callback only
 * while its sphere test finds it in view, so it is run against the picture's sides. */
void func_003136D0(gaddr moby) {
    if (g_effects++ == 0) {
        g_effect_wider = wide_wider_for(&g_fix_orbs, "fix_wide_nanotech_orbs", &g_effect_taller);
    }
    func_003136D0__game(moby);
    g_effects--;
}
