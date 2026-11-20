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

#endif /* OPENRAC_GAME_PROTOS_H */
