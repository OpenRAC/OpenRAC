/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * A stand-in for hostgen's game_protos.h (USA v1.01's names), for unit tests that compile a piece of
 * the shared rac2 host C (port/game/rac2/host) without a disc: only what those pieces call, each
 * defined by the test. */
#ifndef OPENRAC_GAME_PROTOS_H
#define OPENRAC_GAME_PROTOS_H
#include "openrac/guest.h"

/* enemy_copies.c */
gaddr func_0029EFD0(int a0);
void func_0029F338(gaddr a0);
int func_002761F8(gaddr a0, gaddr a1, int a2, gaddr a3, gaddr a4);
float func_002A89A8(gaddr a0, int a1, float a2);
void func_002A6C58__hooked(gaddr a0, int a1, int a2);
gaddr func_L00_00300618__game(int a0);
void func_002A0F48(gaddr a0);

/* weapon_shots.c */
gaddr func_0029EFD0__game(int a0);
gaddr func_002B9030(int a0);
gaddr func_002B9030__game(int a0);
void func_002A05A8(gaddr a0);
void func_002A8E20(gaddr a0, gaddr a1);
gaddr func_002AB1F0(gaddr a0);
gaddr func_L00_003AB9C0__game(
    gaddr a0, gaddr a1, gaddr a2, gaddr a3, float a4, float a5, float a6, float a7
);
gaddr func_L00_00409E48__game(gaddr a0, gaddr a1, gaddr a2, gaddr a3, float a4);
gaddr func_L00_003AA4C8__game(gaddr a0, gaddr a1, gaddr a2, gaddr a3);
gaddr func_L00_003A98E8__game(gaddr a0, gaddr a1, gaddr a2, float a3, float a4, float a5, int a6);
gaddr func_L00_0038D530__game(gaddr a0, gaddr a1);
gaddr func_L00_003EED10__game(int a0, gaddr a1, gaddr a2);
int func_L00_003FE2D0__game(int a0, int a1, int a2);
gaddr func_L00_00398810__game(gaddr a0, gaddr a1);
gaddr func_L00_00387CA8__game(gaddr a0, gaddr a1);

/* weapon_culling.c */
gaddr func_002AF580__game(
    gaddr a0,
    gaddr a1,
    float a2,
    float a3,
    gaddr a4,
    gaddr a5,
    int a6,
    gaddr a7,
    float a8,
    float a9,
    float a10,
    int a11,
    gaddr a12
);
void func_L00_003C3D90__game(gaddr a0);
void func_L00_00392530__game(gaddr a0);

#endif /* OPENRAC_GAME_PROTOS_H */
