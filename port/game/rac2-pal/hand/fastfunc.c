/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/* Going Commando's fastfunc routines (src/game/0027F128.c), in C: hand-written FPU and MMI assembly
 * in the decompilation (ASM_FUNC), which no C compiles to. Each function here was written from what
 * the routine's instructions do, operation for operation and in the same order, so that the PS2's
 * rounding agrees (hostgen.json "ps2_float" lists most of them); handdiff.py checks each against
 * hostgen's own translation of the retail routine.
 *
 * The frame-time factors are the small-data globals at $gp - 0x76E4 .. - 0x76D4. The routines that
 * round to an integer add 0.5 in the accumulator (adda.s of 0.25 to itself, then madd.s) before
 * cvt.w.s, which truncates: x * factor + 0.5, truncated.
 *
 * In v1.01's names: hostgen renames this file for PAL and USA v2.00 ("primary_hand"). */
#include "common.h"

extern float D_001A7990;
extern float D_001A7994;
extern float D_001A7998;
extern float D_001A799C;
extern float D_001A79A0;

float func_00283108(float x) {
    return D_001A7990 * x;
}

int func_00283150(int x) {
    return (int)((float)x * D_001A7990 + 0.5f);
}

/* multiply_global_factor */
float func_00283178(float x) {
    return D_001A7994 * x;
}

int func_00283188(int x) {
    return (int)((float)x * D_001A7994 + 0.5f);
}

float func_002831B0(float x) {
    return D_001A7998 * x;
}

int func_002831C0(int x) {
    return (int)((float)x * D_001A7998 + 0.5f);
}

/* scale_by_frame_time */
float func_002831E8(float x) {
    return D_001A799C * x;
}

float func_002831F8(float x) {
    return D_001A79A0 * x;
}

/* The least of three words (pminw, twice; the low word's sign extended). */
int func_00283518(int a, int b, int c) {
    int m = b < a ? b : a;
    return c < m ? c : m;
}
