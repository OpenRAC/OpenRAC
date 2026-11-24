/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * A game's VU0 scalar sine and cosine, as native functions, for the games
 * whose host C replaces them (rac1/host/hand.c, rac2/host/trig.c,
 * rac3-pal/host/trig.c, rac4-ntsc/host/trig.c): each gives its game's
 * constants and binds its routines to two-line functions that call these.
 *
 * The games' routines move their float argument into VU0 and start a
 * microprogram that a VIF MPG packet of the game loaded. Its cosine entry
 * adds pi/2 and runs on into the sine entry, which folds the angle into
 * [-pi/2, pi/2] with pi - x and -pi - x, then sums the odd series
 * x + c3 x^3 + c5 x^5 + c7 x^7 + c9 x^9. The constants are the immediates of
 * the program's LOI pairs, written into each game's C as numbers (they are
 * data: the console's values, bit for bit); the order of the sum is each
 * game's (below). */
#ifndef OPENRAC_VU0_SINE_H
#define OPENRAC_VU0_SINE_H

#ifdef __cplusplus
extern "C" {
#endif

/* The order in which a game's port sums the series. The orders give different
 * roundings, so each game keeps its own. */
typedef enum openrac_vu0_sine_order {
    /* x + x x^2 (c3 + x^2 (c5 + x^2 (c7 + x^2 c9))): nested in x^2. */
    OPENRAC_VU0_SINE_NESTED,
    /* (x + c3 x^3) + x^4 ((c5 x + c7 x^3) + c9 x^5): two halves joined by x^4. */
    OPENRAC_VU0_SINE_HALVES,
    /* Going Commando's program's own order in the EE's arithmetic (openrac/ps2float.h): the
     * fold, then x^3 .. x^9 by repeated products with x^2, then the accumulator
     * x * 1, + c3 x^3, + c5 x^5, + c7 x^7, + c9 x^9, the cosine's pi/2 added first. */
    OPENRAC_VU0_SINE_EE,
    /* The halves (as OPENRAC_VU0_SINE_HALVES) in the EE's arithmetic, each product and sum
     * cut as the program's: Up Your Arsenal's program sums its series this way. */
    OPENRAC_VU0_SINE_HALVES_EE,
} openrac_vu0_sine_order;

/* A game's sine program: its constants, and how its port sums the series. */
typedef struct openrac_vu0_sine_program {
    float half_pi;                /* The cosine entry's constant, added to the angle. */
    float pi;                     /* The fold's: pi - x above pi/2. */
    float minus_pi;               /* The fold's: -pi - x below -pi/2. */
    float series[4];              /* c3, c5, c7, c9. */
    openrac_vu0_sine_order order; /* How the port sums the series. */
} openrac_vu0_sine_program;

/* Returns the sine of `angle` (radians) as `program` computes it. */
float openrac_vu0_sine(const openrac_vu0_sine_program* program, float angle);

/* Returns the cosine of `angle` (radians): the sine of `angle` plus the
 * program's pi/2. */
float openrac_vu0_cosine(const openrac_vu0_sine_program* program, float angle);

#ifdef __cplusplus
}
#endif

#endif
