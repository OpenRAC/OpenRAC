/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/* The level programs' copies of six fastfunc routines (src/levels/shared/L00_002E5918.c, level 0's
 * places; hostgen gives each level's copy the same C): the same instructions as the boot's
 * func_00283240 .. func_002832E8 (fastfunc.c), which see. Not in "ps2_float": plain host floats, as
 * hostgen's translation of them. In v1.01's names ("primary_hand"). */
#include "common.h"

extern float D_001A7910;
extern float D_001A7914;
extern float D_001A7918;
extern float D_001A791C;
extern float D_001A7920;

/* cvt.w.s: truncated, saturated by sign. A NaN can only come from a factor whose bits the EE reads
 * as a huge number (it has no NaN), and it saturates to that number's sign as the EE's would. */
static int cvt_w_s(float f) {
    union {
        float f;
        int i;
    } u;

    if (f == f) {
        return (int)f;
    }
    u.f = f;
    return u.i < 0 ? (int)0x80000000 : 0x7FFFFFFF;
}

int func_L00_002EE9F8(int x) {
    return cvt_w_s((float)x * D_001A7910 + 0.5f);
}

float func_L00_002EEA20(float x) {
    return D_001A7914 * x;
}

int func_L00_002EEA30(int x) {
    return cvt_w_s((float)x * D_001A7914 + 0.5f);
}

int func_L00_002EEA68(int x) {
    return cvt_w_s((float)x * D_001A7918 + 0.5f);
}

float func_L00_002EEA90(float x) {
    return D_001A791C * x;
}

float func_L00_002EEAA0(float x) {
    return D_001A7920 * x;
}
