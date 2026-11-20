/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/* The level programs' copies of six fastfunc routines (src/levels/shared/L00_002E5918.c, level 0's
 * places; hostgen gives each level's copy the same C): the same instructions as the boot's
 * func_00283150 .. func_002831F8 (fastfunc.c), which see. Not in "ps2_float": plain host floats, as
 * hostgen's translation of them. In v1.01's names ("primary_hand"). */
#include "common.h"

extern float D_001A7990;
extern float D_001A7994;
extern float D_001A7998;
extern float D_001A799C;
extern float D_001A79A0;

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

int func_L00_002EF260(int x) {
    return cvt_w_s((float)x * D_001A7990 + 0.5f);
}

float func_L00_002EF288(float x) {
    return D_001A7994 * x;
}

int func_L00_002EF298(int x) {
    return cvt_w_s((float)x * D_001A7994 + 0.5f);
}

int func_L00_002EF2D0(int x) {
    return cvt_w_s((float)x * D_001A7998 + 0.5f);
}

float func_L00_002EF2F8(float x) {
    return D_001A799C * x;
}

float func_L00_002EF308(float x) {
    return D_001A79A0 * x;
}
