/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Ratchet & Clank: the view tests of the draw callbacks in a picture wider than the game's.
 *
 * Seas, lava and the sand sea (Aridia, Kerwan, Rilgar, Umbris, Batalia, Gaspar, Oltanis: the
 * liquid grid; Novalis's lava and other overlay layers) are drawn by level code, most of it run
 * from the game's four draw callback lists (func_001F4A00, func_001F4BB8, func_001F4A78,
 * func_001F4AF0, each list's runner; callback_lists.c). Each decides what to send from the game's
 * own 4:3 view, read from two places the frame's setup writes:
 *
 * - the camera block (0x187040, func_001F2608): +0x100 a world-to-clip matrix that culls, whose
 *   first terms of each column give x. The liquid grid's cull (func_L02_00250720) tests each 7 x 7
 *   cell block's corners against it: a block whose four corners all have x > |w|, or all x < -|w|,
 *   is not drawn;
 * - the view block (0x18CE00, func_001F3140): its copies of the tangent of half the horizontal
 *   field of view (+0x2C0) and of its secant (+0x2C8, +0x2E0), the clip constants. The bounding
 *   sphere test (func_L00_00200290, FastBSphereCheck) culls a sphere when |x| - sec * r > tan * z;
 *   the overlay layers and other callbacks test with it.
 *
 * The port's picture shows the game's 4:3 view in its middle and opens the horizontal tangent by
 * as much as the picture is wider (frontend.cpp, openrac_game_picture_wider), so what these tests
 * cull at the 4:3 sides is in the picture: tiles near its sides were missing. Here the tests run
 * against values opened by the same amount: the matrix's x terms divided by it, the tangent
 * multiplied by it and the secants made the new tangent's. They are opened for each list's run
 * (for whatever its callbacks test), and again at the two tests themselves when a callback set the
 * view up anew in between (the liquid grid's callback sets its fog first, which builds the
 * matrices again); each time they are put back right after, those still as they were set.
 *
 * The liquid grid's cull only ever decides what is drawn, wherever it is called from. The sphere
 * test also serves the game's own logic (an explosion shakes the camera only when it is in view),
 * so it is opened only while a draw callback list runs, and in the updates below, whose test
 * stands for "can the player see it": everything else, the game's logic among it, sees the 4:3
 * view. At 4:3 nothing changes.
 *
 * Those updates: the nanotech cluster's (class 806, the orb on a nanotech crate, NanotechBallUpdate,
 * func_L00_002D9EA0). Each tick it tests its centre with the sphere test (radius in the sphere's w,
 * draw distance 64) and, only when in view, registers its draw callback (func_L00_002DACC0, the
 * sphere, halo and glass sheen) and spawns its eight glints. Tested against the 4:3 tangent, a crate
 * between the 4:3 edge and the picture's lost its orb, although the crate itself is drawn and
 * "drawn" (moby_view.h). Its state changes (homing in on the hero, the pickup) do not depend on
 * the test. And Kerwan's air traffic's (classes 75, 115-120, 132, 795, the cars, buses and ships;
 * PAL func_L03_0029ED60, US FUN_L03_0029dba8; ReRAC's air_traffic): each tick the first of a group
 * parks the whole group for 5 ticks while the sphere test (through the level's copy of
 * func_L01_00275690, draw distance) finds none of them in view, and a ship not drawn last frame
 * tests itself to choose how it turns and whether it puffs. Tested against the 4:3 tangent, the
 * traffic stopped dead between the 4:3 edge and the picture's, in plain view; on the console it
 * stops only out of sight. Opened, it stops out of the picture's sight instead. This one changes
 * what the game does in a wide picture (where the ships are a few ticks later), not only what it
 * draws: the setting turns it off. And the smoke and fire emitter's (class 760, the fires and smoke
 * of Oltanis's buildings and others; PAL func_L00_002D83D8, US FUN_L00_002d6f28): each tick it
 * returns at once while the sphere test finds it out of view, before it spawns its flames and
 * registers their draw callback. Tested against the 4:3 tangent, a fire between the 4:3 edge and
 * the picture's went out. Its spawning draws random numbers, so this one too changes what the game
 * does in a wide picture (the random sequence the rest of the game sees), and the setting turns
 * it off.
 *
 * They are game fixes the player can switch back to the console's 4:3 view (differences.json;
 * openrac_game_fix, game_host.h): fix_wide_liquids for the lists' runs and the liquid grid's cull,
 * fix_wide_nanotech_orbs for the cluster's update, fix_wide_air_traffic for the traffic's,
 * fix_wide_fires for the emitter's. Off, they run as the game wrote them.
 *
 * Not opened: the callbacks' tests that project a point to the screen and compare it with the
 * screen's pixels. The rest of the world is the port's own: it draws every terrain part, tie and
 * shrub and every live moby from the extracted level, whatever the game culled.
 */
#include "rac1.h"

#define WIDE_CAMERA OPENRAC_DATA(RAC1_ADDR(0x00187040u))
#define WIDE_VIEW OPENRAC_DATA(RAC1_ADDR(0x0018CE00u))
#include "../../common/lib/wide_view_lists.h"

/* How many updates whose sphere test stands for what the player sees are running (the nanotech
 * cluster's, Kerwan's air traffic's, the smoke and fire emitter's), and how much wider the picture
 * is while they do. */
static int g_effects;
static float g_effect_wider = 1.0f;

static const unsigned char* g_fix_liquids;
static const unsigned char* g_fix_orbs;
static const unsigned char* g_fix_traffic;
static const unsigned char* g_fix_fires;

static void run_list(void (*run)(void)) {
    wide_run_list(run, wide_wider_for(&g_fix_liquids, "fix_wide_liquids"));
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
    const float wider = g_lists > 0 ? g_wider : wide_wider_for(&g_fix_liquids, "fix_wide_liquids");
    if (!(wider > 1.0f) || (g_lists > 0 && wide_still_open(&g_list_matrix))) {
        func_L02_00250720__game(grid);
        return;
    }
    wide_opened o;
    wide_open_matrix(&o, wider);
    func_L02_00250720__game(grid);
    wide_close_view(&o);
}

/* FastBSphereCheck: 1 in view, 0 in view but across the guard band's edge, -1 culled. */
int func_L00_00200290(gaddr sphere, float far) {
    return wide_sphere(func_L00_00200290__game, sphere, far, g_effects, g_effect_wider);
}

/* NanotechBallUpdate: the nanotech cluster (the orb on a nanotech crate) draws only while its
 * sphere test finds it in view; it is run against the picture's sides. It is also a hook point for
 * mods (rac1.moby.nanotech_ball.update): the hook sits under this, as func_L00_002D9EA0__hooked
 * (hostgen's hooks.c: the mod's replacement, or the game's C), so the fix holds for a mod's too. */
void func_L00_002D9EA0(gaddr moby) {
    if (g_effects++ == 0) {
        g_effect_wider = wide_wider_for(&g_fix_orbs, "fix_wide_nanotech_orbs");
    }
    func_L00_002D9EA0__hooked(moby);
    g_effects--;
}

/* Kerwan's air traffic's update: a group of ships parks while its sphere test finds none of them in
 * view; it is run against the picture's sides. A hook point for mods too (rac1.moby.train_car.update),
 * under this as func_L03_0029ED60__hooked, so the fix holds for a mod's update as well. */
void func_L03_0029ED60(gaddr moby) {
    if (g_effects++ == 0) {
        g_effect_wider = wide_wider_for(&g_fix_traffic, "fix_wide_air_traffic");
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
        g_effect_wider = wide_wider_for(&g_fix_fires, "fix_wide_fires");
    }
    func_L00_002D83D8__hooked(moby);
    g_effects--;
}
