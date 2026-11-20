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

/* Entry 0xC80, the program's start, which func_0038D6E8 and func_0038D708 (hand-written) start: the
 * Euler angles in vf1 (x, y, z), the rotation rows out in vf20 to vf22, as Going Commando's 0xD18
 * (rac2/host/trig.c, rac2_vu0_rotation_rows: its code opens with the same instructions, scheduled
 * otherwise). From the identity it applies Rx, Ry, then Rz, each step left out when its angle's
 * exponent is zero; a row is R.e_i with R = Rz.Ry.Rx. */
void rac3_vu0_rotation_rows(void) {
    float r[3][3] = {{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
    for (int step = 0; step < 3; ++step) {
        const float angle = OPENRAC_VU0->vf[1][step];
        if (angle == 0.0f) {
            continue;
        }
        const float s = sinf(angle), c = cosf(angle);
        float m[3][3] = {{0}};
        if (step == 0) {
            m[0][0] = 1.0f;
            m[1][1] = c;
            m[1][2] = s;
            m[2][1] = -s;
            m[2][2] = c;
        } else if (step == 1) {
            m[0][0] = c;
            m[0][2] = -s;
            m[1][1] = 1.0f;
            m[2][0] = s;
            m[2][2] = c;
        } else {
            m[0][0] = c;
            m[0][1] = s;
            m[1][0] = -s;
            m[1][1] = c;
            m[2][2] = 1.0f;
        }
        for (int i = 0; i < 3; ++i) {
            float row[3];
            for (int j = 0; j < 3; ++j) {
                row[j] = m[0][j] * r[i][0] + m[1][j] * r[i][1] + m[2][j] * r[i][2];
            }
            for (int j = 0; j < 3; ++j) {
                r[i][j] = row[j];
            }
        }
    }
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            OPENRAC_VU0->vf[20 + i][j] = r[i][j];
        }
        OPENRAC_VU0->vf[20 + i][3] = 0.0f;
    }
    for (int j = 0; j < 4; ++j) {
        OPENRAC_VU0->vf[23][j] = j == 3 ? 1.0f : 0.0f;
    }
}
