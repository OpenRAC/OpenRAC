/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Going Commando's hand-written assembly, in C: routines no C compiles to, which the decompilation
 * keeps as assembly (ASM_FUNC) and which a build without the disc needs all the same. Each was
 * written from what its instructions do, and is bound to its address by libraries.tsv (a "host" row
 * with a signature), in every program that carries a copy of it.
 *
 * The vector routines run the vector unit's macro instructions; the one thing they keep beyond memory
 * is what they leave in its registers, which only the cross product reads back (its fourth field):
 * vf3 is kept here for it. */
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "openrac/game_host.h"
#include "openrac/ee.h"
#include "openrac/ee_ops.h"
#include "openrac/guest.h"
#include "rac2.h"

/* The quadword an address lies in: lqc2 and sqc2 ignore the address's low four bits. */
static float* quad(gaddr p) {
    return (float*)G(p & ~(gaddr)15u);
}

/* vf3 of the vector unit, as the routines here leave it. */
static float vf3[4];

/* sceSifWriteBackDCache (0x0011CEC8): writes the data cache back over [p, p + size), 64 bytes a
 * line. The host has no cache to write back. */
void func_0011CEC8(gaddr p, int size) {
    (void)p;
    (void)size;
}

/* Calls a callback record's function with its argument when it has one: {function, argument}
 * (0x0011DDD8). */
void func_0011DDD8(int unused, gaddr record) {
    (void)unused;
    const gaddr fn = GREF(gaddr, record);
    if (fn != 0) {
        GFN(int (*)(int), fn)(GREF(int, record + 4u));
    }
}

/* A thread-wakeup wrapper (0x0011B8D8): a stub in every build of the port so far, kept so. */
void func_0011B8D8(int thread) {
    (void)thread;
}

/* A thread-wakeup handler: wakes the thread its third argument names (0x00127B18). */
void func_00127B18(int unused, unsigned short also_unused, gaddr thread) {
    (void)unused;
    (void)also_unused;
    func_0011B8D8((int)thread);
}

/* The movie decoder's colour planes to bytes: 24 rows of 16 halfwords each, every one clamped to
 * 0..255 (pminh, pmaxh with the quadword of 0x00FF halfwords after the routine) and packed (ppacb).
 * func_0012B0D8 adds two blocks first (paddh, wrapping in 16 bits); func_0012B138 has one. */
static uint8_t clamp_byte(int16_t v) {
    return (uint8_t)(v < 0 ? 0 : v > 255 ? 255 : v);
}

void func_0012B0D8(gaddr out, gaddr a, gaddr b) {
    for (int i = 0; i < 24 * 16; ++i) {
        const int16_t sum = (int16_t)(GREF(int16_t, a + 2u * (gaddr)i) + GREF(int16_t, b + 2u * (gaddr)i));
        GREF(uint8_t, out + (gaddr)i) = clamp_byte(sum);
    }
}

void func_0012B138(gaddr out, gaddr a) {
    for (int i = 0; i < 24 * 16; ++i) {
        GREF(uint8_t, out + (gaddr)i) = clamp_byte(GREF(int16_t, a + 2u * (gaddr)i));
    }
}

/* fastfunc: an unconditional trap (teq $zero, $zero), the game's assert. */
void func_00282A28(void) {
    static openrac_reach_site site = {"func_00282A28", "trap", "the game's assert", 0, 0};
    openrac_guest_reach(&site);
}

/* fastfunc: x clamped to [lo, hi] as max then min (max.s, min.s). */
float func_00282C68(float x, float lo, float hi) {
    const float m = x > lo ? x : lo;
    return m < hi ? m : hi;
}

/* fastfunc: out = a * b in all four fields (vmul). */
void func_00282D78(gaddr out, gaddr a, gaddr b) {
    const float* x = quad(a);
    const float* y = quad(b);
    const float r[4] = {x[0] * y[0], x[1] * y[1], x[2] * y[2], x[3] * y[3]};
    memcpy(quad(out), r, sizeof r);
}

/* fastfunc: out = b x a in x, y and z (vopmula, vopmsub); w is what vf3 held. */
void func_00282DD8(gaddr out, gaddr a, gaddr b) {
    const float* x = quad(a);
    const float* y = quad(b);
    const float ax = x[0], ay = x[1], az = x[2];
    const float bx = y[0], by = y[1], bz = y[2];
    vf3[0] = by * az - ay * bz;
    vf3[1] = bz * ax - az * bx;
    vf3[2] = bx * ay - ax * by;
    memcpy(quad(out), vf3, sizeof vf3);
}

/* sceVu0Normalize (the second entry of func_001290BC): out = v / |v| in x, y and z, w 0 (vmul,
 * vaddy, vaddz, vsqrt, vdiv, vmulq). */
void func_001290E0(gaddr out, gaddr v) {
    const float* x = quad(v);
    const float len = sqrtf(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
    const float q = 1.0f / len;
    const float r[4] = {x[0] * q, x[1] * q, x[2] * q, 0.0f};
    memcpy(quad(out), r, sizeof r);
}

/* sceVu0ScaleVector (the second entry of func_00129120): out = v * s in all four fields (vmulx). */
void func_00129148(gaddr out, gaddr v, float s) {
    const float* x = quad(v);
    const float r[4] = {x[0] * s, x[1] * s, x[2] * s, x[3] * s};
    memcpy(quad(out), r, sizeof r);
}

/* Copies count quadwords from src to dst through the scratchpad (0x0029CFB0): at most 1024 at a
 * time, to scratchpad address 0 (retail's toSPR transfer, channel 9) and back out (fromSPR, channel
 * 8), each over before the next starts, as the routine waits for it. */
void func_0029CFB0(gaddr dst, gaddr src, int count) {
    while (count > 0) {
        const int n = count < 0x400 ? count : 0x400;
        const size_t bytes = (size_t)n * 16u;
        memmove(G(GUEST_SCRATCHPAD), G(src & 0x01FFFFF0u), bytes);
        memmove(G(dst & 0x01FFFFF0u), G(GUEST_SCRATCHPAD), bytes);
        src += (gaddr)bytes;
        dst += (gaddr)bytes;
        count -= n;
    }
}

/* Level 0's vector routines (and their copies in other levels). */
/* out = max(a, b) and min(a, b) in all four fields (vmax, vmini). */
void func_L00_002EE058(gaddr out, gaddr a, gaddr b) {
    const float* x = quad(a);
    const float* y = quad(b);
    float r[4];
    for (int i = 0; i < 4; ++i) {
        r[i] = x[i] > y[i] ? x[i] : y[i];
    }
    memcpy(quad(out), r, sizeof r);
}

void func_L00_002EE078(gaddr out, gaddr a, gaddr b) {
    const float* x = quad(a);
    const float* y = quad(b);
    float r[4];
    for (int i = 0; i < 4; ++i) {
        r[i] = x[i] < y[i] ? x[i] : y[i];
    }
    memcpy(quad(out), r, sizeof r);
}

/* Clears twelve bytes: a doubleword and a word (sd $0, sw $0). */
void func_L00_002EE0C0(gaddr p) {
    GREF(uint64_t, p) = 0;
    GREF(uint32_t, p + 8u) = 0;
}

/* a -= b in x, y and z, in place; w is a's (vsub.xyz). The first argument is not read. */
void func_L04_0030EDE0(gaddr unused, gaddr a, gaddr b) {
    (void)unused;
    float* x = quad(a);
    const float* y = quad(b);
    const float r[3] = {x[0] - y[0], x[1] - y[1], x[2] - y[2]};
    memcpy(x, r, sizeof r);
}

/* Oozla's animated texture (level 1, 0x003042AC): 4096 texels, each the blend of two frames' texels,
 * out = trunc(clut[a[i]] * (1 - f) + clut[b[i]] * f) in each of R, G, B and A. The palette is
 * a2's 256 colours, read as the GS stores a CSM1 palette (in every 32 entries, 8..15 and 16..23
 * swap places) and widened to floats; the routine reads it into the scratchpad twice, for both
 * frames, from a2 both times. a1 and a3 are the frames' indices; a4 is not read. count is the
 * number of texels (4096), a multiple of four. */
void func_L01_003042AC(gaddr out, gaddr a, gaddr clut, gaddr b, gaddr unused, int count, float f) {
    (void)unused;
    float colours[256][4];
    for (int i = 0; i < 256; ++i) {
        const int group = i & ~31, k = i & 31;
        const int from = group + (k >= 8 && k < 16 ? k + 8 : k >= 16 && k < 24 ? k - 8 : k);
        const uint8_t* c = (const uint8_t*)G(clut + 4u * (gaddr)from);
        for (int lane = 0; lane < 4; ++lane) {
            colours[i][lane] = (float)c[lane];
        }
    }
    const float keep = 1.0f - f;
    for (int i = 0; i < count; ++i) {
        const float* x = colours[GREF(uint8_t, a + (gaddr)i)];
        const float* y = colours[GREF(uint8_t, b + (gaddr)i)];
        uint8_t* o = (uint8_t*)G(out + 4u * (gaddr)i);
        for (int lane = 0; lane < 4; ++lane) {
            /* Two products, then their sum (vmulw, vmulx, vadd): no fused multiply-add. */
            volatile float p = x[lane] * keep;
            volatile float q = y[lane] * f;
            o[lane] = (uint8_t)(int32_t)(p + q);
        }
    }
}

/* fdlibm's __ieee754_fmod (0x00117848): x - n * y with n = trunc(x / y), exactly, as every fmod
 * is. Its C reads a two-entry table of signed zeros (Zero[]) that has no address of its own. */
double func_00117848(double x, double y) {
    return fmod(x, y);
}

/* A routine that returns 0 (0x0026F7A0: jr $ra; daddu $v0, $zero, $zero). Its C is the decompilation's,
 * but USA v2.00's catalogue does not list its copy there (at 0x0026F7A8, among same-looking stubs), so
 * the port gives it here, for every release alike. */
unsigned int func_0026F7A0(void) {
    return 0;
}

/* sceVu0UnitMatrix (the second entry of func_00129160, 0x00129178): m = the identity. The library stores
 * (0,0,0,1) and its vmr32 rotations, the row at +0x30 first; no caller reads m in between. */
void func_00129178(gaddr m) {
    float* r = (float*)G(m);
    for (int i = 0; i < 16; ++i) {
        r[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    }
}

/* The vector unit's sine kernel (0x001291A0), called by sceVu0's rotation helpers (func_00129218,
 * func_001292C0, func_00129368) with its argument and result in the unit's registers: the angle in
 * vf6.x, the result in vf4.x. A polynomial in x^2 with the four coefficients at 0x00137EA0 (vf5),
 * summed from the highest term down in vf4.x, then cos from it as +/- sqrt(1 - s^2) by a3 (zero:
 * plus). Each step is a macro instruction of the routine, in its order, with what the unit does to
 * a denormal result (flushed to a signed zero); the registers it leaves are part of its result
 * (vf4, vf5, vf6, vf7, vf8, Q). Not "ps2_float": host floats, as hostgen's translation of it. */
static float vu_flush(float x) {
    return openrac_ee_vu_flush(x);
}

int func_001291A0(int a0, int a1, int a2, int a3) {
    (void)a0;
    (void)a1;
    (void)a2;
    openrac_vu0_state* vu = OPENRAC_VU0;
    float (*vf)[4] = vu->vf;
    const float* k = quad(RAC2_ADDR(0x00137EA0u));
    for (int i = 0; i < 4; ++i) {
        vf[5][i] = k[i];
    }
    vf[6][3] = vf[6][0];                      /* vmr: x kept in w */
    vf[4][0] = vu_flush(0.0f + vf[6][0]);     /* vaddx.x vf4, vf0, vf6 */
    vf[6][0] = vu_flush(vf[6][0] * vf[6][0]); /* x^2 */
    for (int i = 1; i < 4; ++i) {
        vf[4][i] = vu_flush(vf[4][i] * 0.0f); /* vmulx.yzw vf4, vf4, vf0 */
    }
    float t[4];
    for (int i = 0; i < 4; ++i) {
        t[i] = vf[5][i] * vf[6][3]; /* coefficients times x */
    }
    for (int i = 0; i < 4; ++i) {
        vf[8][i] = vu_flush(t[i]);
    }
    for (int i = 0; i < 3; ++i) {
        vf[5][i] = vu_flush(0.0f - 0.0f); /* vsub vf5, vf0, vf0 */
    }
    vf[5][3] = vu_flush(1.0f - 1.0f);
    /* Each term gets one more x^2 per step down, and is added to the sum as it is finished. */
    for (int i = 0; i < 4; ++i) {
        t[i] = vf[8][i] * vf[6][0];
    }
    for (int i = 0; i < 4; ++i) {
        vf[8][i] = vu_flush(t[i]);
    }
    for (int n = 3; n >= 1; --n) {
        for (int i = 0; i < n; ++i) {
            t[i] = vf[8][i] * vf[6][0];
        }
        for (int i = 0; i < n; ++i) {
            vf[8][i] = vu_flush(t[i]);
        }
        vf[4][0] = vu_flush(vf[4][0] + vf[8][n]);
    }
    vf[4][0] = vu_flush(vf[4][0] + vf[8][0]);
    t[0] = vf[5][0] + vf[4][0];
    t[1] = vf[5][1] + vf[4][0];
    vf[4][0] = vu_flush(t[0]);
    vf[4][1] = vu_flush(t[1]);
    vf[7][0] = vu_flush(vf[4][0] * vf[4][0]);
    vf[7][3] = vu_flush(1.0f - vf[7][0]);
    vu->q = openrac_ee_sqrt(vf[7][3]);
    vf[7][0] = vu_flush(0.0f + vu->q);
    vf[4][0] = vu_flush(a3 != 0 ? vf[5][0] - vf[7][0] : vf[5][0] + vf[7][0]);
    return 0;
}
