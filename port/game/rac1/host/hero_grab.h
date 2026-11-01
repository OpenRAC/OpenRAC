/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Ratchet & Clank: how a held hero goes to where he is held (hero_grab.c), apart from game memory so
 * that a test can run it (port/tests/hero_grab_test.c).
 *
 * The place he is held at comes over a network, some times a second, and the first of them may be a
 * few units from where he stood. He goes there by a share of the way a tick, so that neither
 * shows as a snap; a place further off than GRAB_SNAP (a holder that moved on while he was on his way,
 * a respawn) is taken at once, as there is nothing to see of the way.
 */
#ifndef RAC1_HERO_GRAB_H
#define RAC1_HERO_GRAB_H

#define GRAB_SHARE 0.2f
#define GRAB_SNAP 12.0f

/* The hero at `at` one tick nearer to `wanted`. */
static inline void grab_step(float at[3], const float wanted[3]) {
    float d2 = 0.0f;
    for (int i = 0; i < 3; ++i) {
        d2 += (wanted[i] - at[i]) * (wanted[i] - at[i]);
    }
    const float share = d2 > GRAB_SNAP * GRAB_SNAP ? 1.0f : GRAB_SHARE;
    for (int i = 0; i < 3; ++i) {
        at[i] += (wanted[i] - at[i]) * share;
    }
}

#endif
