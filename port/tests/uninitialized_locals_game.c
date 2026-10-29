/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * replenish_hero_drones' loop as hostgen writes it (the decompilation's func_L00_002CF170: the
 * moby it passes to spawn_hero_drone is set only by the call before), compiled with the games' C
 * options (cmake/Games.cmake) and kept apart from the test so that clang sees only the call.
 */
#include <stdint.h>

typedef uint32_t gaddr;

gaddr uninitialized_spawn(int previous, gaddr pos);

void uninitialized_replenish(unsigned char alive, gaddr pos) {
    gaddr m;
    int i;
    for ((i = (int)alive); (i < 6); (i++)) {
        (m = (gaddr)((gaddr)(uninitialized_spawn((int)(m), pos))));
    }
}
