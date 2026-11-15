/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Ratchet & Clank: whether a moby is in view, the test MobyProc makes before it draws one
 * (render_native.c, mark_drawn_mobys), apart from game memory so that a test can run it
 * (port/tests/moby_view_test.c).
 *
 * Its verdict is the moby's +0x31, "drawn this frame", which the game reads back as logic (the moby
 * loop, the target list, many classes' updates). So it is the console's test against the game's
 * own 4:3 view, whatever shape and field of view the port's picture has.
 */
#ifndef OPENRAC_RAC1_MOBY_VIEW_H
#define OPENRAC_RAC1_MOBY_VIEW_H

#include <math.h>

/*
 * `d` the sphere's centre less the camera's (world units), `rows` the camera's 3 x 4 rows (z, x,
 * y: forward, side, up), `r` the radius, `distance` the draw distance, `tx` and `ty` the tangents
 * of half the game's field of view. In view: within the draw distance, not wholly behind the near
 * plane, not wholly outside a side plane.
 */
static inline int moby_in_view(
    const float d[3], const float rows[12], float r, float distance, float tx, float ty
) {
    const float kx = sqrtf(1.0f + tx * tx), ky = sqrtf(1.0f + ty * ty);
    const float z = d[0] * rows[0] + d[1] * rows[1] + d[2] * rows[2];
    const float x = d[0] * rows[4] + d[1] * rows[5] + d[2] * rows[6];
    const float y = d[0] * rows[8] + d[1] * rows[9] + d[2] * rows[10];
    return z <= distance && z + r > 32.0f / 1024.0f && fabsf(x) - r * kx <= tx * z
           && fabsf(y) - r * ky <= ty * z;
}

#endif
