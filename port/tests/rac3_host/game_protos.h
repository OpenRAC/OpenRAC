/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * A stand-in for hostgen's game_protos.h (PAL's names), for unit tests that compile a piece of the
 * shared rac3 host C (port/game/rac3-pal/host) without a disc: the pieces declare what they call
 * themselves, and each test defines it. */
#ifndef OPENRAC_GAME_PROTOS_H
#define OPENRAC_GAME_PROTOS_H
#include "openrac/guest.h"

/* The game's own bodies of the draw routines render.c wraps, which hostgen declares. */
void func_003A7EF8__game(gaddr a0);
void func_003A6B90__game(gaddr a0);
void func_L01_004AD768__game(gaddr a0);
void func_L01_004B04D8__game(gaddr a0);
void func_003D98C0__game(gaddr a0, int a1, int a2, int a3, float a4, float a5, float a6);

#endif
