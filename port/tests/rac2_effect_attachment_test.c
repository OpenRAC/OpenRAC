/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Tests the RAC2 draw callback attachment classifier against synthetic moby records. Class and
 * state meanings come from func_003136D0 and func_L00_003C3D90 (the shield's moby); no disc data
 * is used.
 */
#include <stdio.h>

#include "draw_owner.h"

/* A synthetic 0x100-byte moby record, addressed through the guest-memory contract. */
_Alignas(16) static uint8_t ram[0x200];
uint8_t* openrac_guest_base = ram;

/* Checks intact and floating anchors, excluding fly-in particles and non-moby callbacks. */
int main(void) {
    const gaddr ball = 0x100;
    GREF(uint16_t, ball + 0xAA) = 806;

    /* States 1 and 2 share a stable position anchor (seen in game code: func_003136D0). */
    const gaddr anchors[] = {0, ball, ball, 0};
    for (unsigned state = 0; state < 4; ++state) {
        GREF(uint8_t, ball + 0x20) = (uint8_t)state;
        if (rac2_effect_attachment(ball) != anchors[state]) {
            return 1;
        }
    }

    /* Invalid callback arguments must never become anchors. */
    GREF(uint8_t, ball + 0x20) = 1;
    if (rac2_effect_attachment(0) != 0 || rac2_effect_attachment(1) != 0
        || rac2_effect_attachment(0xFFFFFFFFu) != 0) {
        return 1;
    }

    /* A floating orb remains its own anchor after its former crate has been deleted. */
    GREF(uint8_t, ball + 0x20) = 2;
    GREF(gaddr, ball + 0xB8) = 0xFFFFFFFFu;
    if (rac2_effect_attachment(ball) != ball) {
        return 1;
    }

    GREF(uint16_t, ball + 0xAA) = 501;
    if (rac2_effect_attachment(ball) != 0) {
        return 1;
    }

    /* The strips' anchors: the nanotech ball turned with it, the hero, his weapon and his shield
     * by their position alone, any other callback's (a trail, a beam in the world) none. */
    const gaddr hero = 0x080, hand = 0x180;
    GREF(uint16_t, ball + 0xAA) = 806;
    GREF(uint8_t, ball + 0x20) = 1;
    rac2_strip_anchor a = rac2_strip_anchor_of(ball, hero, hand);
    if (a.moby != ball || a.position_only) {
        return 1;
    }
    a = rac2_strip_anchor_of(hero, hero, hand);
    if (a.moby != hero || !a.position_only) {
        return 1;
    }
    a = rac2_strip_anchor_of(hand, hero, hand);
    if (a.moby != hand || !a.position_only) {
        return 1;
    }
    const uint16_t shields[] = {RAC2_SHIELD_CHARGER_CLASS, RAC2_TESLA_BARRIER_CLASS};
    for (unsigned i = 0; i < 2; ++i) {
        GREF(uint16_t, ball + 0xAA) = shields[i];
        GREF(uint8_t, ball + 0x20) = 1;
        a = rac2_strip_anchor_of(ball, hero, hand);
        if (a.moby != ball || !a.position_only) {
            return 1;
        }
        /* Gone (dead or free): none. */
        GREF(uint8_t, ball + 0x20) = 0xFE;
        if (rac2_strip_anchor_of(ball, hero, hand).moby != 0) {
            return 1;
        }
    }
    GREF(uint16_t, ball + 0xAA) = 1490;
    GREF(uint8_t, ball + 0x20) = 1;
    if (rac2_strip_anchor_of(ball, hero, hand).moby != 0 || rac2_strip_anchor_of(0, 0, 0).moby != 0
        || rac2_strip_anchor_of(0xFFFFFFFFu, hero, hand).moby != 0) {
        return 1;
    }

    puts("RAC2 effect attachment: OK");
    return 0;
}
