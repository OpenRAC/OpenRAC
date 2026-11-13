/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Ratchet & Clank: the view tests of the draw callbacks in a picture wider than the game's.
 *
 * The problem. Seas, lava and the sand sea (Aridia, Kerwan, Rilgar, Umbris, Batalia, Gaspar,
 * Oltanis: the liquid grid; Novalis's lava and other overlay layers) are drawn by level code, most
 * of it run from the game's four draw callback lists (func_001F4A00, func_001F4BB8, func_001F4A78,
 * func_001F4AF0, each list's runner; callback_lists.c). Each callback culls against the game's own
 * 4:3 view, which it reads from two places the frame's setup writes:
 *
 * - the camera block (0x187040, func_001F2608): +0x100 is a world-to-clip matrix that culls; the
 *   first terms of each column give x. The liquid grid's cull (func_L02_00250720) tests the
 *   corners of each 7 x 7 cell block against it: a block whose four corners all have x > |w|, or
 *   all x < -|w|, is not drawn;
 * - the view block (0x18CE00, func_001F3140): the tangent of half the horizontal field of view
 *   (+0x2C0), its secant (+0x2C8, +0x2E0) and the clip constants. The bounding sphere test
 *   (func_L00_00200290, FastBSphereCheck) culls a sphere when |x| - sec * r > tan * z; the overlay
 *   layers and other callbacks use it.
 *
 * The port shows the 4:3 view in the middle of the picture and opens the horizontal tangent by as
 * much as the picture is wider (program/game_host.cpp, openrac_game_picture_wider). So what these
 * tests cull at the 4:3 sides is in the picture, and tiles near its sides were missing.
 *
 * The fix. The tests run against values opened by the same amount: the matrix's x terms divided by
 * it, the tangent multiplied by it, and the secants made the new tangent's. They are opened for
 * each list's run, and again at the two tests themselves when a callback set the view up anew in
 * between (the liquid grid's callback sets its fog first, which builds the matrices again). Each
 * time they are put back right after. At 4:3 nothing changes.
 *
 * The liquid grid's cull only ever decides what is drawn. The sphere test also serves the game's
 * logic (an explosion shakes the camera only when it is in view), so it is opened only while a draw
 * callback list runs, and in the updates below, whose test means "can the player see it".
 * Everything else, the game's logic among it, sees the 4:3 view.
 *
 * - The nanotech cluster (class 806, the orb on a nanotech crate; NanotechBallUpdate,
 *   func_L00_002D9EA0). Each tick it tests its centre with the sphere test (radius in the sphere's
 *   w, draw distance 64). Only when in view does it register its draw callback (func_L00_002DACC0:
 *   the sphere, halo and glass sheen) and spawn its eight glints. Against the 4:3 tangent, a crate
 *   between the 4:3 edge and the picture's edge lost its orb, although the crate itself is drawn
 *   (moby_view.h). Its state (homing in on the hero, the pickup) does not depend on the test.
 * - Kerwan's air traffic (classes 75, 115-120, 132, 795: the cars, buses and ships; PAL
 *   func_L03_0029ED60, US FUN_L03_0029dba8; ReRAC's air_traffic). Each tick the first of a group
 *   parks the whole group for 5 ticks while the sphere test (through the level's copy of
 *   func_L01_00275690, draw distance) finds none of them in view. A ship not drawn last frame also
 *   tests itself to choose how it turns and whether it puffs. Against the 4:3 tangent, the traffic
 *   stopped dead in plain view; on the console it stops only out of sight. This one changes what
 *   the game does in a wide picture (where the ships are a few ticks later), not only what it
 *   draws.
 * - The smoke and fire emitter (class 760, the fires and smoke of Oltanis's buildings and others;
 *   PAL func_L00_002D83D8, US FUN_L00_002d6f28). Each tick it returns at once while the sphere
 *   test finds it out of view, before it spawns its flames and registers their draw callback.
 *   Against the 4:3 tangent, a fire near the picture's edge went out. Its spawning draws random
 *   numbers, so this one also changes what the game does (the random sequence the rest of the game
 *   sees).
 * - The water grids' updates (pools and tanks: Rilgar's class 831, PAL func_L05_0030DED8; Umbris's
 *   water plates, func_L07_0030D8E8; Pokitaru's class 1158, func_L11_0030F4F8; Hoven's class 19,
 *   func_L12_002C0310; Gemlik's classes 1263 and 1393, func_L13_0030B340). Each tick each grid
 *   tests itself (func_L01_00275690, radius 48 or 64) and shows none of its tiles while out of
 *   view. Against the 4:3 tangent, a pool's water vanished past the 4:3 edge (Qwark's course on
 *   Umbris). The test only sets which tiles are drawn (and so whose ripples step).
 *
 * Each is a game fix the player can switch back to the console's 4:3 view (differences.json;
 * openrac_game_fix, game_host.h); off, the code runs as the game wrote it:
 *
 * - fix_wide_liquids: the lists' runs, the liquid grid's cull and the water grids' updates;
 * - fix_wide_nanotech_orbs: the cluster's update;
 * - fix_wide_air_traffic: the traffic's;
 * - fix_wide_fires: the emitter's.
 *
 * Not opened: the callbacks' tests that project a point to the screen and compare it with the
 * screen's pixels. The rest of the world is the port's own: it draws every terrain part, tie, shrub
 * and live moby from the extracted level, whatever the game culled.
 */
#include "rac1.h"

#define WIDE_CAMERA OPENRAC_DATA(RAC1_ADDR(0x00187040u))
#define WIDE_VIEW OPENRAC_DATA(RAC1_ADDR(0x0018CE00u))
#include "../../common/lib/wide_view_lists.h"

/* How many updates whose sphere test stands for what the player sees are running (the nanotech
 * cluster's, Kerwan's air traffic's, the smoke and fire emitter's, the water grids'), and how much
 * wider the picture is while they do. */
static int g_effects;
static float g_effect_wider = 1.0f;
static float g_effect_taller = 1.0f;

static const unsigned char* g_fix_liquids;
static const unsigned char* g_fix_orbs;
static const unsigned char* g_fix_traffic;
static const unsigned char* g_fix_fires;

static void run_list(void (*run)(void)) {
    float taller;
    const float wider = wide_wider_for(&g_fix_liquids, "fix_wide_liquids", &taller);
    wide_run_list(run, wider, taller);
}

/* ExecuteDrawCallbacks: list 1. */
void func_001F4A00(void) {
    run_list(func_001F4A00__game);
}

/* ExecuteDrawCallbacks2: list 2. */
void func_001F4BB8(void) {
    run_list(func_001F4BB8__game);
}

/* ExecuteDrawCallbacks3: list 3. */
void func_001F4A78(void) {
    run_list(func_001F4A78__game);
}

/* ExecuteDrawCallbacks4: list 4. */
void func_001F4AF0(void) {
    run_list(func_001F4AF0__game);
}

/* The liquid grid's cull: a flag per block into the scratchpad (0: not drawn). */
void func_L02_00250720(gaddr grid) {
    float taller = g_taller;
    const float wider =
        g_lists > 0 ? g_wider : wide_wider_for(&g_fix_liquids, "fix_wide_liquids", &taller);
    if (!(wider > 1.0f) || (g_lists > 0 && wide_still_open(&g_list_matrix))) {
        func_L02_00250720__game(grid);
        return;
    }
    wide_opened o;
    wide_open_matrix(&o, wider, taller);
    func_L02_00250720__game(grid);
    wide_close_view(&o);
}

/* FastBSphereCheck: 1 in view, 0 in view but across the guard band's edge, -1 culled. */
int func_L00_00200290(gaddr sphere, float far) {
    return wide_sphere(
        func_L00_00200290__game, sphere, far, g_effects, g_effect_wider, g_effect_taller
    );
}

/* NanotechBallUpdate: the nanotech cluster (the orb on a nanotech crate) draws only while its
 * sphere test finds it in view; it is run against the picture's sides. It is also a hook point for
 * mods (rac1.moby.nanotech_ball.update): the hook sits under this, as func_L00_002D9EA0__hooked
 * (hostgen's hooks.c: the mod's replacement, or the game's C), so the fix holds for a mod's too. */
void func_L00_002D9EA0(gaddr moby) {
    if (g_effects++ == 0) {
        g_effect_wider = wide_wider_for(&g_fix_orbs, "fix_wide_nanotech_orbs", &g_effect_taller);
    }
    func_L00_002D9EA0__hooked(moby);
    g_effects--;
}

/* Kerwan's air traffic's update: a group of ships parks while its sphere test finds none of them in
 * view; it is run against the picture's sides. A hook point for mods too (rac1.moby.train_car.update),
 * under this as func_L03_0029ED60__hooked, so the fix holds for a mod's update as well. */
void func_L03_0029ED60(gaddr moby) {
    if (g_effects++ == 0) {
        g_effect_wider = wide_wider_for(&g_fix_traffic, "fix_wide_air_traffic", &g_effect_taller);
    }
    func_L03_0029ED60__hooked(moby);
    g_effects--;
}

/* The smoke and fire emitter's update: it spawns and draws its flames only while its sphere test
 * finds it in view; it is run against the picture's sides. A hook point for mods too
 * (rac1.moby.particle_field.update), under this as func_L00_002D83D8__hooked, so the fix holds for
 * a mod's update as well. */
void func_L00_002D83D8(gaddr moby) {
    if (g_effects++ == 0) {
        g_effect_wider = wide_wider_for(&g_fix_fires, "fix_wide_fires", &g_effect_taller);
    }
    func_L00_002D83D8__hooked(moby);
    g_effects--;
}

/* A water grid's update: it shows its tiles only while its sphere test finds it in view; it is run
 * against the picture's sides. Each is a hook point for mods too, under this as <name>__hooked, so
 * the fix holds for a mod's update as well. */
static void water_grid_update(void (*update)(gaddr), gaddr moby) {
    if (g_effects++ == 0) {
        g_effect_wider = wide_wider_for(&g_fix_liquids, "fix_wide_liquids", &g_effect_taller);
    }
    update(moby);
    g_effects--;
}

/* Rilgar's water (class 831). */
void func_L05_0030DED8(gaddr moby) {
    water_grid_update(func_L05_0030DED8__hooked, moby);
}

/* Umbris's water plates (classes 902, 941, 943, 944 and more). */
void func_L07_0030D8E8(gaddr moby) {
    water_grid_update(func_L07_0030D8E8__hooked, moby);
}

/* Pokitaru's water tiles (class 1158). */
void func_L11_0030F4F8(gaddr moby) {
    water_grid_update(func_L11_0030F4F8__hooked, moby);
}

/* Hoven's water tiles (class 19). */
void func_L12_002C0310(gaddr moby) {
    water_grid_update(func_L12_002C0310__hooked, moby);
}

/* Gemlik's bobbing pool tiles (classes 1263, 1393). */
void func_L13_0030B340(gaddr moby) {
    water_grid_update(func_L13_0030B340__hooked, moby);
}
