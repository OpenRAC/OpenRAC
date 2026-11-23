/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Going Commando's VU0 scalar trigonometry.
 *
 * 0x00283180 and 0x00283198 move their float argument into VU0 and call the
 * microprogram that the VIF MPG packet at 0x0010E9EC loads at 0xC80 (94
 * instruction pairs): entry 0xC80 adds pi/2 (cosine) and runs on into entry
 * 0xC90, the sine. It folds the angle into [-pi/2, pi/2] with pi - x and
 * -pi - x, then sums the odd series x + c3 x^3 + c5 x^5 + c7 x^7 + c9 x^9 with
 * the constants the program loads through its I register (0xCF0..0xD08). The
 * port computes it natively: the constants are written here as numbers (the
 * same words on every Going Commando disc, and Ratchet & Clank's), and the sum
 * runs in the program's own order with the EE's arithmetic
 * (common/lib/vu0_sine.h, OPENRAC_VU0_SINE_EE). Every other routine that
 * starts the program (euler_to_matrix and the moby matrices, on its Euler rows
 * at 0xD18) has C of its own, so no VU0 register carries anything into it.
 */
#include "game/common/lib/vu0_sine.h"

/* The microprogram's constants (its LOI pairs): pi/2 at 0xC80 (0x3FC90FDA), pi at 0xC90
 * (0x40490FDA), -pi at 0xC98 (0xC0490FDA) and c3..c9 at 0xCF0..0xD08 (0xBE2AAAA4, 0x3C08873E,
 * 0xB94FB21D, 0x362E9C14). */
static const openrac_vu0_sine_program program = {
    .half_pi = 0x1.921fb4p+0f,
    .pi = 0x1.921fb4p+1f,
    .minus_pi = -0x1.921fb4p+1f,
    .series = {-0x1.555548p-3f, 0x1.110e7cp-7f, -0x1.9f643ap-13f, 0x1.5d3828p-19f},
    .order = OPENRAC_VU0_SINE_EE,
};

float func_00283180(float angle) {
    return openrac_vu0_cosine(&program, angle);
}

float func_00283198(float angle) {
    return openrac_vu0_sine(&program, angle);
}
