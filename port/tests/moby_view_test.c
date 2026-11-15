/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Whether a moby is "drawn" for the game (+0x31): the console's 4:3 test (moby_view.h). */
#include <stdio.h>
#include <stdlib.h>
#include "../game/rac1/host/moby_view.h"
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "%d: %s\n", __LINE__, #c); exit(1); } } while (0)

/* The camera looking along +z of its rows: forward z, side x, up y. */
static const float rows[12] = {0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0};

int main(void) {
    const float tx = 0.63f, ty = 0.63f * 0.75f;
    /* In the middle: in view. */
    const float middle[3] = {0.0f, 0.0f, 10.0f};
    CHECK(moby_in_view(middle, rows, 0.5f, 64.0f, tx, ty));
    /* Between the 4:3 edge (x = 6.3 at z = 10) and a 16:9 one (x = 8.4): out, on both sides, as on
     * the console. */
    const float right[3] = {7.5f, 0.0f, 10.0f}, left[3] = {-7.5f, 0.0f, 10.0f};
    CHECK(!moby_in_view(right, rows, 0.5f, 64.0f, tx, ty));
    CHECK(!moby_in_view(left, rows, 0.5f, 64.0f, tx, ty));
    /* Its sphere reaching over the edge: in. */
    CHECK(moby_in_view(right, rows, 2.0f, 64.0f, tx, ty));
    /* Above the view: out. */
    const float above[3] = {0.0f, 6.0f, 10.0f};
    CHECK(!moby_in_view(above, rows, 0.5f, 64.0f, tx, ty));
    /* Draw distance and near plane. */
    const float far[3] = {0.0f, 0.0f, 70.0f}, behind[3] = {0.0f, 0.0f, -2.0f};
    CHECK(!moby_in_view(far, rows, 0.5f, 64.0f, tx, ty));
    CHECK(!moby_in_view(behind, rows, 0.5f, 64.0f, tx, ty));
    return 0;
}
