/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Ratchet & Clank: follow-camera routines the port answers itself (libraries.tsv rows: the port's
 * version decides the prototype callers see).
 */
#include "rac1.h"

#if !RAC1_OMIT(func_001EC210)
/* func_001EC210 (Camera_handleCollWithHero, games/rac1/src/game/camera.c:97). The executable's copy
 * calls the empty stub func_001E97C8; each level's copy calls the level's create_camera_collision_moby
 * (func_L00_002A6070: a camera_object, class 1007, at the eye), which the hero meets when he faces the
 * camera (the runners' camera proxy). The C's call went to a return-0 place of the stub, so the moby
 * was never made. */
void func_001EC210(gaddr cam) {
    const gaddr follow = OPENRAC_DATA(RAC1_ADDR(0x00187040u)) + 0x190u;
    gaddr* moby = &GREF(gaddr, follow + 0xC4u);
    if (GREF(int16_t, cam + 0x86u) == 0) {
        if (*moby == 0 && openrac_guest_overlay() >= 0) {
            *moby = func_L00_002A6070(follow - 0x50u); /* &gCamera.pos */
        }
    } else if (*moby != 0) {
        func_0020D678(*moby); /* DeleteMoby */
        *moby = 0;
    }
}
#endif
