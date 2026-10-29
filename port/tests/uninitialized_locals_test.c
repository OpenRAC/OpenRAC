/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * A loop that passes a local it has not set yet still runs under the games' C options
 * (cmake/Games.cmake: -ftrivial-auto-var-init=zero). Without it clang drops the loop, as it did
 * replenish_hero_drones' (no drones from the Drone Device).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef uint32_t gaddr;

void uninitialized_replenish(unsigned char alive, gaddr pos);

static int spawned;
static int first_previous = -1;

gaddr uninitialized_spawn(int previous, gaddr pos) {
    if (spawned == 0) {
        first_previous = previous;
    }
    ++spawned;
    return pos + (gaddr)spawned;
}

int main(void) {
    /* Two drones alive: four more are made. */
    uninitialized_replenish(2, 0x01000000u);
    if (spawned != 4) {
        fprintf(stderr, "uninitialized_locals: %d drones made, 4 expected\n", spawned);
        return 1;
    }
    /* The unset moby the first call is given reads as zero. */
    if (first_previous != 0) {
        fprintf(stderr, "uninitialized_locals: the first call was given %d, 0 expected\n",
                first_previous);
        return 1;
    }
    return 0;
}
