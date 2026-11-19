/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 * Only the translated entry points called by the ship-destination bridge. */
#pragma once
/* Stands in for game/rac1/host/rac1.h (ship_destination_unit.c includes this one first). */
#define OPENRAC_RAC1_H
#include "openrac/game_host.h"
#define RAC1_ADDR(a) a
int func_00222848__game(gaddr menu);
int func_0021BB90__game(gaddr menu);
void func_L00_00290030__game(int target);
void func_0022F4A0(int target);
