/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Ratchet & Clank: where the hero stands beside a point (hero_place.c), apart from game memory so
 * that a test can run it (port/tests/hero_place_test.c).
 *
 * The ring. `anchor` is where another player stands. The first spot looked at is kPlaceRing units
 * behind `yaw`, the rest around the ring from there; each must be reachable from the anchor at waist
 * height with nothing in the way (no spot behind a wall or a door), have ground within kPlaceStep
 * of the anchor's feet (no spot off a ledge or up on a crate) and room for the hero over that
 * ground. The hero looks at the anchor from wherever he lands. No spot on the ring: the anchor
 * itself, the other player being the proof that a hero can stand there. No ground under the anchor
 * either (it stood on a moby that has moved on): no place at all.
 */
#ifndef OPENRAC_RAC1_HERO_PLACE_H
#define OPENRAC_RAC1_HERO_PLACE_H

#include <math.h>

#define PLACE_SPOTS 8
#define PLACE_RING 1.5f
#define PLACE_STEP 1.0f
#define PLACE_WAIST 1.0f
#define PLACE_HEIGHT 1.6f
#define PLACE_PI 3.14159265f

/* Nothing between two points; the ground under a point, into *z, looked for from PLACE_STEP above
 * it to PLACE_STEP below it. The port's are the game's line queries. */
typedef int (*place_clear_fn)(void* user, const float from[3], const float to[3]);
typedef int (*place_ground_fn)(void* user, const float at[3], float* z);

/* The spot beside `anchor` into `out`, and the way the hero faces there into `facing`: 1, or 0 for
 * no place. */
static inline int place_beside(
    const float anchor[3],
    float yaw,
    place_clear_fn clear,
    place_ground_fn ground,
    void* user,
    float out[3],
    float* facing
) {
    float feet;
    if (!ground(user, anchor, &feet)) {
        return 0;
    }
    const float from[3] = {anchor[0], anchor[1], feet + PLACE_WAIST};
    for (int i = 0; i < PLACE_SPOTS; ++i) {
        /* Behind, then back and forth around the ring: the sides before the front. */
        const int turn = (i + 1) / 2 * (i % 2 != 0 ? 1 : -1);
        const float angle = yaw + PLACE_PI + (float)turn * 2.0f * PLACE_PI / PLACE_SPOTS;
        const float spot[3] = {
            anchor[0] + PLACE_RING * cosf(angle), anchor[1] + PLACE_RING * sinf(angle), feet
        };
        const float waist[3] = {spot[0], spot[1], feet + PLACE_WAIST};
        float z;
        if (!clear(user, from, waist) || !ground(user, spot, &z)) {
            continue;
        }
        const float head[3] = {spot[0], spot[1], z + PLACE_HEIGHT};
        const float foot[3] = {spot[0], spot[1], z + 0.1f};
        if (!clear(user, foot, head)) {
            continue;
        }
        out[0] = spot[0];
        out[1] = spot[1];
        out[2] = z;
        *facing = angle + PLACE_PI;
        return 1;
    }
    out[0] = anchor[0];
    out[1] = anchor[1];
    out[2] = feet;
    *facing = yaw;
    return 1;
}

#endif
