/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/*
 * A moby the Sheepinator already turned into a sheep is not turned again (soft lock "sheep-once", T-45).
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
 * The port leaves a target whose link is already 2 alone: no second sheep, no deletion. USA v1.01
 * alone: PAL's and v2.00's copies (src/pal/levels/shared/L00_00408110.c,
 * src/us2/levels/shared/L00_00404C18-cc.c) take a link in mode 2 as they take 1 and keep the robot,
 * so on those discs the wraps only call the game's.
 */
#include <stdint.h>

#include "game_protos.h"
#define RAC2_MOBY_TARGET_RECORD func_002AB1F0
#include "moby_checks.h"
#include "openrac/game_host.h"
#include "rac2.h"

gaddr func_L00_00403730__game(gaddr self);
gaddr func_L00_00408800__game(gaddr self);

#define MOBY_FLAGS 0x34u /* u16; 0x20: it has private values */
#define LINK_MODE 0x0Eu  /* of private value 0 */
#define LINK_TURNED 2    /* turned, its owner takes it back */

static int already_turned(gaddr self) {
#if !defined(RAC2_PRIMARY)
    (void)self;
    return 0;
#else
    if (!rac2_moby_slot(self) || (GREF(uint16_t, self + MOBY_FLAGS) & 0x20u) == 0) {
        return 0;
    }
    /* Its pvars and link followed only when they are memory (moby_checks.h): an NG++ copy's are the
     * port's, and a pvars word may hold a float. */
    const gaddr vars = rac2_moby_pvars(self, 1);
    const gaddr link = vars != 0 ? GREF(gaddr, vars) : 0;
    return rac2_memory_at(link, LINK_MODE + 1u) && GREF(uint8_t, link + LINK_MODE) == LINK_TURNED;
#endif
}

/* sheep_spawn_from_object */
gaddr func_L00_00403730(gaddr self) {
    return already_turned(self) ? 0 : func_L00_00403730__game(self);
}

/* dark_sheep_spawn_from_object */
gaddr func_L00_00408800(gaddr self) {
    return already_turned(self) ? 0 : func_L00_00408800__game(self);
}
