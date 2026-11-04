/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
#include <math.h>

#include "rac1.h"

void func_L00_0025F4A8(
    gaddr owner,
    gaddr unused,
    gaddr position,
    float a,
    float b,
    int burst,
    int sparks,
    int shards,
    float c,
    float d,
    float e,
    float scale,
    int sound,
    float flash,
    int shake,
    int spray,
    int reduction,
    unsigned char flag
) {
    const gaddr centre = position ? position : owner ? owner + 0x10u : 0;
    if (centre && isfinite(scale)) {
        float point[3];
        for (unsigned axis = 0; axis < 3; ++axis) {
            point[axis] = GREF(float, centre + axis * 4u);
        }
        const float radius = fminf(4.f, fmaxf(.5f, a > 0 ? a : fabsf(scale) * 1.5f));
        const float strength = fminf(2.f, fmaxf(.1f, fabsf(scale) * .8f));
        openrac_game_water_explosion(point, radius, strength);
    }
    func_L00_0025F4A8__game(
        owner,
        unused,
        position,
        a,
        b,
        burst,
        sparks,
        shards,
        c,
        d,
        e,
        scale,
        sound,
        flash,
        shake,
        spray,
        reduction,
        flag
    );
}
