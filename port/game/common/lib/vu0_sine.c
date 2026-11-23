/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * A game's VU0 scalar sine and cosine (vu0_sine.h): the fold and the orders of
 * the series, on the constants each game gives. The games' C is compiled
 * without fused multiply-add (cmake/Games.cmake), and so is this, so each host
 * order rounds as the game's port always has. */
#include "game/common/lib/vu0_sine.h"

#include "openrac/ps2float.h"

/* Returns x + c3 x^3 + c5 x^5 + c7 x^7 + c9 x^9, nested in x^2. */
static float series_nested(float x, const float c[4]) {
    const float x2 = x * x;
    return x + x * x2 * (c[0] + x2 * (c[1] + x2 * (c[2] + x2 * c[3])));
}

/* Returns x + c3 x^3 + c5 x^5 + c7 x^7 + c9 x^9 as two halves joined by x^4:
 * the products of x and the constants first, then the sums. */
static float series_halves(float x, const float c[4]) {
    const float x2 = x * x;
    const float x4 = x2 * x2;
    const float p7 = x * c[2];
    const float p9 = x * c[3];
    const float p3 = x * c[0];
    float accumulator = x * c[1];
    accumulator = accumulator + p7 * x2;
    const float high = accumulator + p9 * x4;
    accumulator = x + p3 * x2;
    return accumulator + high * x4;
}

/* The fold in the EE's arithmetic: min(x, pi - x), then max with -pi - x. */
static float fold_ee(const openrac_vu0_sine_program* program, float angle) {
    const float upper = openrac_ps2_sub(program->pi, angle);
    const float lower = openrac_ps2_sub(program->minus_pi, angle);
    const float x = openrac_ps2_lt(angle, upper) ? angle : upper;
    return openrac_ps2_gt(x, lower) ? x : lower;
}

/* OPENRAC_VU0_SINE_EE: every sum and product the EE's. */
static float sine_ee(const openrac_vu0_sine_program* program, float angle) {
    const float x = fold_ee(program, angle);
    const float x2 = openrac_ps2_mul(x, x);
    const float x3 = openrac_ps2_mul(x, x2);
    const float x5 = openrac_ps2_mul(x3, x2);
    const float x7 = openrac_ps2_mul(x5, x2);
    const float x9 = openrac_ps2_mul(x7, x2);
    float accumulator = openrac_ps2_mul(x, 1.0f);
    accumulator = openrac_ps2_add(accumulator, openrac_ps2_mul(x3, program->series[0]));
    accumulator = openrac_ps2_add(accumulator, openrac_ps2_mul(x5, program->series[1]));
    accumulator = openrac_ps2_add(accumulator, openrac_ps2_mul(x7, program->series[2]));
    return openrac_ps2_add(accumulator, openrac_ps2_mul(x9, program->series[3]));
}

/* OPENRAC_VU0_SINE_HALVES_EE: the halves, every sum and product the EE's. */
static float sine_halves_ee(const openrac_vu0_sine_program* program, float angle) {
    const float* c = program->series;
    const float x = fold_ee(program, angle);
    const float x2 = openrac_ps2_mul(x, x);
    const float x4 = openrac_ps2_mul(x2, x2);
    const float p7 = openrac_ps2_mul(x, c[2]);
    const float p9 = openrac_ps2_mul(x, c[3]);
    const float p3 = openrac_ps2_mul(x, c[0]);
    float accumulator = openrac_ps2_mul(x, c[1]);
    accumulator = openrac_ps2_add(accumulator, openrac_ps2_mul(p7, x2));
    const float high = openrac_ps2_add(accumulator, openrac_ps2_mul(p9, x4));
    accumulator = openrac_ps2_add(openrac_ps2_mul(x, 1.0f), openrac_ps2_mul(p3, x2));
    return openrac_ps2_add(accumulator, openrac_ps2_mul(high, x4));
}

float openrac_vu0_sine(const openrac_vu0_sine_program* program, float angle) {
    switch (program->order) {
        case OPENRAC_VU0_SINE_EE:
            return sine_ee(program, angle);
        case OPENRAC_VU0_SINE_HALVES_EE:
            return sine_halves_ee(program, angle);
        case OPENRAC_VU0_SINE_NESTED:
        case OPENRAC_VU0_SINE_HALVES:
            break;
    }
    /* The fold: pi - x above pi/2, -pi - x below -pi/2. */
    const float upper = program->pi - angle;
    const float lower = program->minus_pi - angle;
    float x = angle < upper ? angle : upper;
    x = x > lower ? x : lower;
    /* Each game's port keeps the order it has always summed in. */
    return program->order == OPENRAC_VU0_SINE_HALVES ? series_halves(x, program->series)
                                                     : series_nested(x, program->series);
}

float openrac_vu0_cosine(const openrac_vu0_sine_program* program, float angle) {
    if (program->order == OPENRAC_VU0_SINE_EE || program->order == OPENRAC_VU0_SINE_HALVES_EE) {
        return openrac_vu0_sine(program, openrac_ps2_add(angle, program->half_pi));
    }
    return openrac_vu0_sine(program, angle + program->half_pi);
}
