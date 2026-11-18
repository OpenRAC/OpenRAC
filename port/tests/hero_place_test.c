/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Where the hero is put beside another player (hero_place.h), on a made-up level. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../game/rac1/host/hero_place.h"
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "%d: %s\n", __LINE__, #c); exit(1); } } while (0)

/* The level: a floor at z 0, with a pit past pit_x, a wall along x = wall_x, a roof at roof_z (off
 * at 0) over x < 0. Nothing under the anchor at all when `no_ground`. */
typedef struct {
    float pit_x, wall_x, roof_z;
    int no_ground;
    int closed;
} Level;

static int clear(void* user, const float from[3], const float to[3]) {
    const Level* l = user;
    if (l->closed) {
        return 0;
    }
    if ((from[0] < l->wall_x) != (to[0] < l->wall_x)) {
        return 0;
    }
    return !(l->roof_z != 0.0f && to[0] < 0.0f && to[2] >= l->roof_z);
}

static int ground(void* user, const float at[3], float* z) {
    const Level* l = user;
    if (l->no_ground || at[0] > l->pit_x || fabsf(at[2]) > PLACE_STEP) {
        return 0;
    }
    *z = 0.0f;
    return 1;
}

static float distance(const float a[3], const float b[3]) {
    return hypotf(a[0] - b[0], a[1] - b[1]);
}

int main(void) {
    const float anchor[3] = {0.0f, 0.0f, 0.0f};
    float out[3], facing;

    /* Open ground: right behind him, looking the way he looks. */
    Level open = {100.0f, 100.0f, 0.0f, 0};
    CHECK(place_beside(anchor, 0.0f, clear, ground, &open, out, &facing));
    CHECK(fabsf(out[0] + PLACE_RING) < 0.01f && fabsf(out[1]) < 0.01f && out[2] == 0.0f);
    CHECK(fabsf(cosf(facing) - 1.0f) < 0.01f);

    /* Another yaw: another spot (a second joiner's), still looking at him. */
    CHECK(place_beside(anchor, PLACE_PI / 2.0f, clear, ground, &open, out, &facing));
    CHECK(fabsf(out[1] + PLACE_RING) < 0.01f && fabsf(out[0]) < 0.01f);
    CHECK(fabsf(sinf(facing) - 1.0f) < 0.01f);

    /* A wall behind him: a spot on his side of it, never through it. */
    Level walled = {100.0f, -0.5f, 0.0f, 0};
    CHECK(place_beside(anchor, 0.0f, clear, ground, &walled, out, &facing));
    CHECK(out[0] > -0.5f);
    CHECK(fabsf(distance(out, anchor) - PLACE_RING) < 0.01f);

    /* He stands by a pit behind him (yaw pi looks away from it): not off the edge. */
    Level pit = {0.5f, 100.0f, 0.0f, 0};
    CHECK(place_beside(anchor, PLACE_PI, clear, ground, &pit, out, &facing));
    CHECK(out[0] <= 0.5f);

    /* A low roof behind him: no room there. */
    Level low = {100.0f, 100.0f, 1.2f, 0};
    CHECK(place_beside(anchor, 0.0f, clear, ground, &low, out, &facing));
    CHECK(out[0] >= 0.0f);

    /* Walled in on every side: his own spot. */
    Level closet = {100.0f, 100.0f, 0.0f, 0, 1};
    CHECK(place_beside(anchor, 0.0f, clear, ground, &closet, out, &facing));
    CHECK(out[0] == anchor[0] && out[1] == anchor[1]);

    /* No ground under him: no place. */
    Level none = {100.0f, 100.0f, 0.0f, 1};
    CHECK(!place_beside(anchor, 0.0f, clear, ground, &none, out, &facing));
    return 0;
}
