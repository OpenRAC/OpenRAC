/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/* rac1.h for rac1_hud_panels_test.c: the shared C's PAL v2.00 names as the NTSC table gives them.
 * The test includes it before game/rac1/host/hud_panels.c, so the real rac1.h stays out. */
#pragma once
#define OPENRAC_RAC1_H
#include "game_protos.h"
#include "openrac/game_host.h"

#define RAC1_ADDR(a) RAC1_ADDR_##a
#define RAC1_LADDR(n, a) RAC1_ADDR_L##n##_##a
#define RAC1_ADDR_0x0013F450u 0x0013F350u
#define RAC1_ADDR_0x0015EEA0u 0x0015EDA0u
#define RAC1_ADDR_0x0015EE80u 0x0015ED80u
#define RAC1_ADDR_L00_0x0015F8D0u 0x0015F7F0u
#define RAC1_ADDR_L00_0x0015F8E8u 0x0015F808u
#define RAC1_ADDR_L00_0x0015F8F0u 0x0015F810u
#define RAC1_ADDR_L00_0x0015F8FCu 0x0015F81Cu
#define RAC1_ADDR_L00_0x0015F900u 0x0015F820u
#define RAC1_TYPE_func_001F5800_u64 long long
#define func_001F5800 game_FUN_001f5450
#define func_00200198 game_FUN_001ff960
#define func_00200248 game_FUN_001ffa10
#define func_L00_0023A1A0 game_FUN_L00_00239810
#define func_L00_0023A1A0__game game_FUN_L00_00239810__game
