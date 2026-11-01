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
 * the constants the program loads through its I register (0xCF0..0xD08).
 * The constants are read from the packet the game itself loaded; the sum is
 * the port's own order of operations, not a matching claim
 * (common/lib/vu0_sine.h).
 */
#include <stdint.h>
#include <string.h>

#include "game/common/lib/vu0_sine.h"
#include "openrac/ee.h"

/* The microprogram's constants: pi/2 at 0xC80, pi at 0xC90, -pi at 0xC98 and
 * c3..c9 at 0xCF0..0xD08. The packet's code starts one word after its MPG tag. */
static const openrac_vu0_sine_program program = {
    .code = 0x0010E9F0u,
    .load = 0xC80u,
    .cosine_entry = 0xC80u,
    .sine_entry = 0xC90u,
    .minus_pi = 0xC98u,
    .series = {0xCF0u, 0xCF8u, 0xD00u, 0xD08u},
    .order = OPENRAC_VU0_SINE_NESTED,
};

float func_00283180(float angle) {
    return openrac_vu0_cosine(&program, angle);
}

float func_00283198(float angle) {
    return openrac_vu0_sine(&program, angle);
}

/* The same microprogram started directly by hand-written code (func_L02_00364A18): the angle in
 * vf1.x, the result in vf1.x. */
void rac2_vu0_cosine(void) {
    OPENRAC_VU0->vf[1][0] = func_00283180(OPENRAC_VU0->vf[1][0]);
}

void rac2_vu0_sine(void) {
    OPENRAC_VU0->vf[1][0] = func_00283198(OPENRAC_VU0->vf[1][0]);
}

/*
 * The microprogram at 0xD18, which euler_to_matrix (0x00283410) and the moby matrices (0x002A0F48)
 * start: the Euler angles in vf1 (x, y, z), rotation rows out in vf20 to vf22 and (0, 0, 0, 1) in
 * vf23. From the identity it applies Rx, Ry, then Rz, each step left out when its angle's exponent
 * is zero; a row is R.e_i with R = Rz.Ry.Rx (as games/rac1's nonmatching/game/func_001FA218.c reads
 * the same program, the same bytes in both games: docs/port/VU_PROGRAMS.md, 28259).
 */
void rac2_vu0_rotation_rows(void) {
    float r[3][3] = {{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
    for (int step = 0; step < 3; ++step) {
        const float angle = OPENRAC_VU0->vf[1][step];
        uint32_t bits;
        memcpy(&bits, &angle, sizeof bits);
        if ((bits & 0x7F800000u) == 0) {
            continue;
        }
        const float s = func_00283198(angle), c = func_00283180(angle);
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
            memcpy(r[i], row, sizeof row);
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
