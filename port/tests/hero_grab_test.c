/* SPDX-License-Identifier: GPL-3.0-or-later */
/* How a held hero goes to where he is held (hero_grab.h). */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "../game/rac1/host/hero_grab.h"
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "%d: %s\n", __LINE__, #c); exit(1); } } while (0)

static float distance(const float a[3], const float b[3]) {
    return sqrtf((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1])
                 + (a[2] - b[2]) * (a[2] - b[2]));
}

int main(void) {
    /* A few units off: a share of the way, no tick taking more than the share. */
    float at[3] = {0.0f, 0.0f, 0.0f};
    const float near[3] = {3.0f, 4.0f, 0.0f};
    grab_step(at, near);
    CHECK(fabsf(distance(at, near) - 5.0f * (1.0f - GRAB_SHARE)) < 0.001f);
    /* And there within a third of a second. */
    for (int tick = 0; tick < 20; ++tick) {
        grab_step(at, near);
    }
    CHECK(distance(at, near) < 0.05f);
    /* Standing there, he stays. */
    grab_step(at, at);
    CHECK(distance(at, near) < 0.05f);
    /* Further than a blend could hide: at once. */
    const float far[3] = {3.0f, 4.0f, GRAB_SNAP + 1.0f};
    grab_step(at, far);
    CHECK(distance(at, far) < 0.001f);
    puts("ok");
    return 0;
}
