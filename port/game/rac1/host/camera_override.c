/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Ratchet & Clank: a camera given from outside, for looking at one exact view (OPENRAC_CAMERA,
 * docs/port/DEBUGGING.md; port/tools/regress routes' "camera").
 *
 * Every view the game draws with is built by BuildViewMatrices (func_001F2608; US FUN_001f2260):
 * from the camera block's eye (0x187040 + 0x140) and rotation rows forward, left, up (0x187390)
 * it makes the view, the clip and cull matrices and the sky's view, and the window reads the same
 * eye and rows for the world it draws itself (game_state.cpp). A player's report gives the eye
 * and the forward the debug drawer showed; turning the right stick to it is never exact. With
 * OPENRAC_CAMERA=X,Y,Z,FX,FY,FZ the eye and rows are written just before each build, so the
 * game's matrices, the sky and the window's world all see that camera. The rows are made without
 * roll, as the follow camera's are: left is the world's up crossed with forward, up is forward
 * crossed with left. The game's logic reads the rows too (the hero walks along the view), so
 * this is a debugging switch only, applied in a level only (the menus' panel views are built here
 * too); unset, the build runs as the game wrote it.
 */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "openrac/host_env.h"
#include "rac1.h"

#define CAMERA_POS RAC1_ADDR(0x00187180u)
#define CAMERA_ROWS RAC1_ADDR(0x00187390u)

void func_001F2608(void) {
    static int read = 0, on = 0;
    static float rows[12], eye[3];
    if (!read) {
        read = 1;
        const char* v = openrac_env_string("OPENRAC_CAMERA");
        float f[3];
        if (v
            && sscanf(v, "%f,%f,%f,%f,%f,%f", &eye[0], &eye[1], &eye[2], &f[0], &f[1], &f[2])
                   == 6) {
            const float n = sqrtf(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
            const float h = sqrtf(f[0] * f[0] + f[1] * f[1]);
            if (n > 0.0f && h > 0.0f) {
                const float fw[3] = {f[0] / n, f[1] / n, f[2] / n};
                const float l[3] = {-f[1] / h, f[0] / h, 0.0f};
                const float u[3] =
                    {fw[1] * l[2] - fw[2] * l[1],
                     fw[2] * l[0] - fw[0] * l[2],
                     fw[0] * l[1] - fw[1] * l[0]};
                memcpy(rows + 0, fw, sizeof(fw));
                memcpy(rows + 4, l, sizeof(l));
                memcpy(rows + 8, u, sizeof(u));
                on = 1;
            }
        }
    }
    /* Only in a level: the boot program's title and menus build their own views here too. */
    if (on && openrac_guest_overlay() >= 0) {
        float* pos = (float*)G(OPENRAC_DATA(CAMERA_POS));
        float* r = (float*)G(OPENRAC_DATA(CAMERA_ROWS));
        memcpy(pos, eye, sizeof(eye));
        for (int i = 0; i < 3; ++i) {
            memcpy(r + 4 * i, rows + 4 * i, 3 * sizeof(float));
        }
    }
    func_001F2608__game();
}
