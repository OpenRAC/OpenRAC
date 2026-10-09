/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The vector library's rotations (libvu0), which the game calls and which
 * are assembly in the decompilation: m0 = the rotation by angle about one
 * axis, applied to m1. A matrix is four rows of four floats, a vector is
 * multiplied on the left (v' = v m), as the game's own matrix code does.
 *
 * To check against the game: the order (rotation applied before or after
 * m1) is read from how the callers use the result; it is the one OpenRAC's
 * tests should pin once a level loads. */
#include <math.h>

#include "game_protos.h"
#include "rac1_host.h"

static void rotate(gaddr out, gaddr in, int axis, float angle) {
    const float c = cosf(angle), s = sinf(angle);
    float r[4][4] = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};
    const int a = (axis + 1) % 3, b = (axis + 2) % 3;
    r[a][a] = c;
    r[a][b] = s;
    r[b][a] = -s;
    r[b][b] = c;
    float m[4][4], result[4][4];
    memcpy(m, G(in), sizeof m);
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result[i][j] =
                m[i][0] * r[0][j] + m[i][1] * r[1][j] + m[i][2] * r[2][j] + m[i][3] * r[3][j];
        }
    }
    memcpy(G(out), result, sizeof result);
}

void func_001253F8(gaddr m0, gaddr m1, float rz) {
    rotate(m0, m1, 2, rz);
} /* sceVu0RotMatrixZ */

void func_001254A0(gaddr m0, gaddr m1, float rx) {
    rotate(m0, m1, 0, rx);
} /* sceVu0RotMatrixX */

void func_00125548(gaddr m0, gaddr m1, float ry) {
    rotate(m0, m1, 1, ry);
} /* sceVu0RotMatrixY */
