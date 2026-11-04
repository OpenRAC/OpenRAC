/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
#ifndef OPENRAC_RAC1_WATER_BOMB_H
#define OPENRAC_RAC1_WATER_BOMB_H
#include "openrac/game_host.h"
#include "openrac/guest.h"

/* The Bomb Glove's class 121 has its own FX path, not SpawnExplosion. Its
 * +0xBC byte changes from zero when it detonates, then advances through the
 * expanding damage ring. Snapshot before the update: an underwater detonation
 * may delete the moby immediately, while a normal one enters state 2.
 * Values remain in the deleted slot until its reuse cooldown expires. */
static inline int rac1_water_bomb_pending(gaddr moby) {
    return GREF(int16_t, moby + 0xA6u) == 121 && GREF(uint8_t, moby + 0x20u) == 1
           && GREF(uint8_t, moby + 0xBCu) == 0;
}

static inline void rac1_water_bomb_after(gaddr moby, int pending) {
    if (!pending || GREF(uint8_t, moby + 0xBCu) == 0) {
        return;
    }
    float point[3];
    for (unsigned axis = 0; axis < 3; ++axis) {
        point[axis] = GREF(float, moby + 0x10u + 4u * axis);
    }
    openrac_game_water_explosion(point, 3.f, 1.6f);
}
#endif
