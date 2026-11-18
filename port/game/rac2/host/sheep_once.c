/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/*
 * A moby the Sheepinator already turned into a sheep is not turned again (fix "sheep-once", T-45).
 *
 * sheep_spawn_from_object (func_L00_00403730) and its dark twin (func_L00_00408800) make the sheep,
 * then look at the target's link (its private value 0, byte 0xE): 1, a fighter its owner takes back,
 * becomes 2 and the owner puts it back to sleep on its next update; any other value deletes the
 * target (moby_mark_spawned). The gun picks its target from a list made earlier in the frame
 * (L11_003D90C0-g8.c skips only deleted mobys), so a Joba arena security robot that went back to
 * sleep this frame, health 0 and link still 2, is picked and turned at once: the second turn deletes
 * it. With the 16 gone, an Impossible Challenge round that wants one never ends. The PS2 game does
 * it too (PCSX2, v1.01: three deleted by round 3).
 *
 * The fix leaves a target whose link is already 2 alone: no second sheep, no deletion.
 */
#include <stdint.h>

#include "game_protos.h"
#include "openrac/game_host.h"
#include "rac2.h"

gaddr func_L00_00403730__game(gaddr self);
gaddr func_L00_00408800__game(gaddr self);

#define MOBY_FLAGS 0x34u /* u16; 0x20: it has private values */
#define MOBY_VARS 0x68u  /* its private values */
#define LINK_MODE 0x0Eu  /* of private value 0 */
#define LINK_TURNED 2    /* turned, its owner takes it back */

static int already_turned(gaddr self) {
    static const unsigned char* fix;
    if (!fix) {
        fix = openrac_game_fix("fix_sheep_once");
    }
    if (!*fix || self == 0 || (GREF(uint16_t, self + MOBY_FLAGS) & 0x20u) == 0) {
        return 0;
    }
    const gaddr link = GREF(gaddr, GREF(gaddr, self + MOBY_VARS));
    return link != 0 && GREF(uint8_t, link + LINK_MODE) == LINK_TURNED;
}

/* sheep_spawn_from_object */
gaddr func_L00_00403730(gaddr self) {
    return already_turned(self) ? 0 : func_L00_00403730__game(self);
}

/* dark_sheep_spawn_from_object */
gaddr func_L00_00408800(gaddr self) {
    return already_turned(self) ? 0 : func_L00_00408800__game(self);
}
