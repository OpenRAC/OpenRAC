/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Two of Going Commando's big hand-written renderers, and the entries inside one of them. The port
 * draws with its own renderers and replaces their callers (host/render.c): MobyAnim's joint pose
 * (func_002A1738) is called only by func_002A4D88, CullAndEmitShrubs (func_002E1228) only by
 * func_002E11C0. func_002E182C and func_002E25C0 are not routines at all: CullAndEmitShrubs runs a
 * block of its code as a subroutine and comes back by a jump to one of them, an address it stored in
 * the scratchpad, with its own registers live. So no build calls any of these, and a build without
 * the disc binds each to a body that reports the reach. */
#include "openrac/game_host.h"
#include "openrac/guest.h"

#define REACHED(name, why)                                                                         \
    static openrac_reach_site site = {name, "unreachable", why, 0, 0};                             \
    openrac_guest_reach(&site)

#define REPLACED "host-replaced renderer: the port never calls it"
#define FRAGMENT                                                                                   \
    "a fragment of func_002E1228, entered only from its assembly; host-replaced renderer"

int func_002A1738(int a0, int a1) {
    (void)a0;
    (void)a1;
    REACHED("func_002A1738", REPLACED);
    return 0;
}

void func_002E1228(void) {
    REACHED("func_002E1228", REPLACED);
}

int func_002E182C(int a0, int a1, int a2, int a3) {
    (void)a0;
    (void)a1;
    (void)a2;
    (void)a3;
    REACHED("func_002E182C", FRAGMENT);
    return 0;
}

int func_002E25C0(void) {
    REACHED("func_002E25C0", FRAGMENT);
    return 0;
}
