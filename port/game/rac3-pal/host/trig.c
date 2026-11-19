/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's VU0 scalar trigonometry.
 *
 * func_0038D478 and func_0038D490 (hand-written, rac3-decomp asm/boot_elf/handwritten) move their
 * float argument into vf1.x and start the VU0 microprogram that the VIF chain at 0x0010F1A0 (its
 * MPG at 0x0010F1AC: 96 instruction pairs at 0xC80, sent by func_003983C8 through KickVif0Chain)
 * loads: entry 0xDA0 is the cosine, 0xDB0 the sine (the callers build a direction as
 * r cos(yaw) cos(pitch), r sin(yaw) cos(pitch), r sin(pitch): func_00382848). The program is laid
 * out otherwise than Going Commando's (rac2/host/trig.c), so its series is not read from it yet:
 * until it is, these are the host's cosf and sinf, close to the console's to a few last places.
 */
#include <math.h>

#include "openrac/ee.h"

float func_0038D478(float angle) {
    return cosf(angle);
}

float func_0038D490(float angle) {
    return sinf(angle);
}

/* The same entries started directly by hand-written code: the angle in vf1.x, the result in
 * vf1.x. */
void rac3_vu0_cosine(void) {
    OPENRAC_VU0->vf[1][0] = cosf(OPENRAC_VU0->vf[1][0]);
}

void rac3_vu0_sine(void) {
    OPENRAC_VU0->vf[1][0] = sinf(OPENRAC_VU0->vf[1][0]);
}
