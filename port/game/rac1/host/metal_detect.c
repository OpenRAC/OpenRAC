/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Ratchet & Clank: the metal detector's dig, told to a mod (game_host.h,
 * openrac_game_metal_detect; interface 1.12, multiplayer: two players must not dig one pile).
 *
 * Moby_MetalDetector_Update (class 585) gives a buried moby's bolts in one tick, inside its update,
 * when the button is held, the item has been out long enough, and its target (class 605, set each
 * tick by the buried moby's own update as the nearest) is under 1.0 away. The same test runs here
 * just before that update, and a mod that says no moves the target to exactly 1.0 for the tick: the
 * detector goes on beeping and gives nothing.
 */
#include "rac1.h"

#define HERO RAC1_ADDR(0x0013F450u)
#define HERO_DETECTOR_READY (HERO + 0x1090u + 0x20u)
#define HERO_DETECTOR_TARGET (HERO + 0x2040u)
#define HERO_DETECTOR_DISTANCE (HERO + 0x2044u)
#define PAD RAC1_ADDR(0x0013CA40u)
#define PAD_HELD (PAD + 0x1A0u)

enum {
    MOBY_STATE = 0x20,
    MOBY_OCLASS = 0xA6,
    CLASS_METAL_DETECTOR = 585,
    CLASS_BURIED_BOLTS = 605,
    STATE_REMOVED = 0xFD,
    STATE_REMOVING = 0xFE,
    BUTTON = 0x20,
    READY_TICKS = 15
};

void rac1_metal_detect(gaddr moby) {
    if (openrac_game_metal_detect == NULL
        || GREF(int16_t, moby + MOBY_OCLASS) != CLASS_METAL_DETECTOR) {
        return;
    }
    const gaddr target = GREF(gaddr, HERO_DETECTOR_TARGET);
    if (target == 0 || !(GREF(float, HERO_DETECTOR_DISTANCE) < 1.0f)
        || GREF(int16_t, target + MOBY_OCLASS) != CLASS_BURIED_BOLTS
        || GREF(uint8_t, target + MOBY_STATE) == STATE_REMOVED
        || GREF(uint8_t, target + MOBY_STATE) == STATE_REMOVING
        || !(GREF(int32_t, PAD_HELD) & BUTTON)
        || GREF(int32_t, HERO_DETECTOR_READY) <= READY_TICKS) {
        return;
    }
    if (!openrac_game_metal_detect(rac1_moby_slot(target), rac1_world_moby_key(target))) {
        GREF(float, HERO_DETECTOR_DISTANCE) = 1.0f;
    }
}
