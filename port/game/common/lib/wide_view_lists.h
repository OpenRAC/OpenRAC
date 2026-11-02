/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * The view tests of the draw callbacks in a picture wider than the game's, what the games with the
 * same camera and view blocks share (rac1/host/wide_view.c says what the tests are, and why; rac2-ntsc/
 * host/wide_view.c). The includer defines WIDE_CAMERA, the camera block (+0x100 the cull matrix), and
 * WIDE_VIEW, the view block (+0x2C0 the clip constants' tangent, +0x2C8 and +0x2E0 its secant), both
 * as the loaded program's addresses, and wraps its list runners and its sphere test with these.
 */
#ifndef OPENRAC_WIDE_VIEW_LISTS_H
#define OPENRAC_WIDE_VIEW_LISTS_H

#include <stdint.h>
#include <string.h>

#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "wide_view.h"

enum {
    WIDE_CULL_MATRIX = 0x100,  /* camera block: the cull matrix */
    WIDE_CLIP_TAN_X = 0x2C0,   /* view block: the clip constants' tangent ... */
    WIDE_CLIP_SEC_X = 0x2C8,   /* ... its secant ... */
    WIDE_CLIP_SCALE_X = 0x2E0, /* ... and the secant again (the sphere test's) */
    WIDE_SLOTS = 4
};

/* Words opened for a test or a list's run: where, what they were, what they were set to. */
typedef struct {
    gaddr at[WIDE_SLOTS];
    uint32_t was[WIDE_SLOTS];
    uint32_t set[WIDE_SLOTS];
    int count;
} wide_opened;

static float wide_get(gaddr at) {
    float f;
    memcpy(&f, G(at), 4);
    return f;
}

static void wide_put(wide_opened* o, gaddr at, float value) {
    uint32_t bits;
    memcpy(&bits, &value, 4);
    o->at[o->count] = at;
    o->was[o->count] = GREF(uint32_t, at);
    o->set[o->count] = bits;
    GREF(uint32_t, at) = bits;
    o->count++;
}

/* Back as they were, those still as they were set (a test that set the view up again keeps it). */
static void wide_close_view(const wide_opened* o) {
    for (int i = o->count - 1; i >= 0; i--) {
        if (GREF(uint32_t, o->at[i]) == o->set[i]) {
            GREF(uint32_t, o->at[i]) = o->was[i];
        }
    }
}

/* The cull matrix's x terms, divided by `wider`. */
static void wide_open_matrix(wide_opened* o, float wider) {
    const gaddr matrix = (WIDE_CAMERA) + WIDE_CULL_MATRIX;
    o->count = 0;
    for (int column = 0; column < 4; column++) {
        const gaddr at = matrix + (gaddr)column * 16u;
        wide_put(o, at, wide_get(at) / wider);
    }
}

/* The clip constants' horizontal tangent, times `wider`, and its secants. */
static void wide_open_clip(wide_opened* o, float wider) {
    const gaddr view = WIDE_VIEW;
    float tan_x, sec_x;
    wide_clip(wide_get(view + WIDE_CLIP_TAN_X), wider, &tan_x, &sec_x);
    o->count = 0;
    wide_put(o, view + WIDE_CLIP_TAN_X, tan_x);
    wide_put(o, view + WIDE_CLIP_SEC_X, sec_x);
    wide_put(o, view + WIDE_CLIP_SCALE_X, sec_x);
}

/* Whether every word `o` opened still holds what it was set to. */
static int wide_still_open(const wide_opened* o) {
    for (int i = 0; i < o->count; i++) {
        if (GREF(uint32_t, o->at[i]) != o->set[i]) {
            return 0;
        }
    }
    return o->count > 0;
}

/* How many draw callback lists are running (a callback could run a list); how much wider than 4:3
 * the picture is while they do (asked once a run, not at every test); the words opened for the
 * whole run. */
static int g_lists;
static float g_wider = 1.0f;
static wide_opened g_list_matrix, g_list_clip;

/* How much wider than 4:3 the picture is for the game fix `key`: 1 (the game's own view) while
 * the player has it off. `*flag` is its flag, asked for the first time. */
static float wide_wider_for(const unsigned char** flag, const char* key) {
    if (!*flag) {
        *flag = openrac_game_fix(key);
    }
    return **flag ? openrac_game_picture_wider() : 1.0f;
}

/* A list's run, with the view opened by `wider` for the callbacks' tests. */
static void wide_run_list(void (*run)(void), float wider) {
    if (g_lists > 0) {
        g_lists++;
        run();
        g_lists--;
        return;
    }
    g_wider = wider;
    if (!(g_wider > 1.0f)) {
        run();
        return;
    }
    wide_open_matrix(&g_list_matrix, g_wider);
    wide_open_clip(&g_list_clip, g_wider);
    g_lists = 1;
    run();
    g_lists = 0;
    wide_close_view(&g_list_clip);
    wide_close_view(&g_list_matrix);
    g_list_matrix.count = g_list_clip.count = 0;
}

/* The sphere test `game`: against the opened view while a list runs, or `effects` updates that
 * stand for what the player sees (at `effect_wider`) do. */
static int wide_sphere(
    int (*game)(gaddr, float), gaddr sphere, float far, int effects, float effect_wider
) {
    const float wider = wide_sphere_wider(g_lists, g_wider, effects, effect_wider);
    if (!(wider > 1.0f) || (g_lists > 0 && wide_still_open(&g_list_clip))) {
        return game(sphere, far);
    }
    wide_opened o;
    wide_open_clip(&o, wider);
    const int result = game(sphere, far);
    wide_close_view(&o);
    return result;
}

#endif
