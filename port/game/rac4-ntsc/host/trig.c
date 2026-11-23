/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Deadlocked VU0 scalar trigonometry, reconstructed from its retail patch.
 * The VIF MPG packet at 0x0010F1E0 loads 148 instruction pairs at 0xB10.
 * Entry 0xC78 shifts x by pi/2; 0xC88 evaluates the sine polynomial.
 * This is a native implementation, not a matching claim
 * (common/lib/vu0_sine.h).
 */
#include "game/common/lib/vu0_sine.h"

/* The patch's constants (its LOI pairs), as numbers: pi/2 at 0xC78 (0x3FC90FDB), pi at 0xC88
 * (0x40490FDB), and c7, c9, c3, c5 at 0xCC0, 0xCC8, 0xCD0, 0xCE0 (0xB94FB21F, 0x362E9C14,
 * 0xBE2AAAA4, 0x3C08873E). It has no -pi of its own: pi negated. */
static const openrac_vu0_sine_program program = {
    .half_pi = 0x1.921fb6p+0f,
    .pi = 0x1.921fb6p+1f,
    .minus_pi = -0x1.921fb6p+1f,
    .series = {-0x1.555548p-3f, 0x1.110e7cp-7f, -0x1.9f643ep-13f, 0x1.5d3828p-19f},
    .order = OPENRAC_VU0_SINE_HALVES,
};

float func_00453928(float angle) {
    return openrac_vu0_cosine(&program, angle);
}

float func_00453940(float angle) {
    return openrac_vu0_sine(&program, angle);
}
