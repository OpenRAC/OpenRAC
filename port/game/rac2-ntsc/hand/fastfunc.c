/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/* Going Commando's fastfunc routines (src/game/0027F128.c), in C: hand-written FPU and vector unit
 * assembly in the decompilation (ASM_FUNC), which no C compiles to. Each function here was written
 * from what the routine's instructions do, operation for operation and in the same order, so that
 * the PS2's rounding agrees (hostgen.json "ps2_float" lists them); handdiff.py checks each against
 * hostgen's own translation of the retail routine.
 *
 * In v1.01's names: hostgen renames this file for PAL and USA v2.00 ("primary_hand"). */
#include "common.h"

extern float D_001A7910; /* a frame-time factor ($gp - 0x76E0) */
extern float D_001A7918; /* the frame time ($gp - 0x76D8) */

/* multiply_global_factor: x times the factor (lwc1, mul.s). */
float func_002828B8(float x)
{
    return D_001A7910 * x;
}

/* scale_by_frame_time: x times the frame time (lwc1, mul.s). */
float func_00282928(float x)
{
    return D_001A7918 * x;
}
