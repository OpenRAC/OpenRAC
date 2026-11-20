/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The port's include/vu0.h (hostgen.json "headers": found before the decompilation's own). The
 * decompilation's header writes the game's vector-library helpers as inline assembly for the
 * console's compiler (VU0 macro-mode instructions on its "j" register class, abs.s, sqrt.s,
 * max.s, plzcw, cvt.w.s, pextlh, psraw), which Clang does not read; here each helper has the same
 * name and parameters and does in C what its instructions do, so hostgen translates the C that
 * calls them like any other, with the helpers inlined where they are called.
 *
 * The add and subtract helpers keep the first source's w: retail's results go to the register the
 * first source was loaded into (vsub.xyz $vf1, $vf1, $vf2), and sqc2 stores the whole quadword. */
#ifndef VU0_H
#define VU0_H

typedef int vu0_q __attribute__((mode(TI)));

typedef struct {
    float x, y, z, w;
} __attribute__((aligned(16))) vu0_v4;

static __inline__ void vec_sub_ab(void* d, void* a, void* b) {
    vu0_v4 *pa = (vu0_v4*)a, *pb = (vu0_v4*)b, r;
    r.x = pa->x - pb->x;
    r.y = pa->y - pb->y;
    r.z = pa->z - pb->z;
    r.w = pa->w;
    *(vu0_v4*)d = r;
}

static __inline__ void vec_sub_ba(void* d, void* a, void* b) {
    vec_sub_ab(d, a, b);
}

static __inline__ void vec_add_ab(void* d, void* a, void* b) {
    vu0_v4 *pa = (vu0_v4*)a, *pb = (vu0_v4*)b, r;
    r.x = pa->x + pb->x;
    r.y = pa->y + pb->y;
    r.z = pa->z + pb->z;
    r.w = pa->w;
    *(vu0_v4*)d = r;
}

static __inline__ void vec_add_ba(void* d, void* a, void* b) {
    vec_add_ab(d, a, b);
}

/* vsub.xyz from $vf0 (zero): the result in a's register, so its w is a's. */
static __inline__ void vec_neg(void* d, void* a) {
    vu0_v4 *pa = (vu0_v4*)a, r;
    r.x = 0.0f - pa->x;
    r.y = 0.0f - pa->y;
    r.z = 0.0f - pa->z;
    r.w = pa->w;
    *(vu0_v4*)d = r;
}

static __inline__ void vec_copy(void* d, void* s) {
    *(vu0_v4*)d = *(vu0_v4*)s;
}

#define qzero(d) (*(vu0_q*)(d) = 0)

static __inline__ float fabs_asm(float x) {
    return x < 0.0f ? -x : x;
}

static __inline__ float sqrt_asm(float x) {
    return __builtin_sqrtf(x);
}

static __inline__ float fmax_asm(float a, float b) {
    return a > b ? a : b;
}

/* blezl x, 1f; neg x, x: negated when not above zero. */
static __inline__ int abs_asm(int x) {
    return x <= 0 ? -x : x;
}

/* plzcw: the leading bits equal to the sign bit, less one. */
static __inline__ int plzcw_asm(int x) {
    unsigned int v = x < 0 ? ~(unsigned int)x : (unsigned int)x;
    int n = 0;
    while (n < 31 && !(v & (0x40000000u >> n))) {
        n++;
    }
    return n;
}

/* cvt.w.s and cvt.s.w on a value that stays in the FPU: the integer is carried in a float's bits. */
static __inline__ float cvtws_asm(float x) {
    union {
        float f;
        int i;
    } u;

    u.i = (int)x;
    return u.f;
}

static __inline__ float cvtsw_asm(float x) {
    union {
        float f;
        int i;
    } u;

    u.f = x;
    return (float)u.i;
}

/* pextlh rd, a, b: the low four halfwords of b and a, interleaved (b's first). */
static __inline__ vu0_q vu0_pextlh(vu0_q a, vu0_q b) {
    vu0_q r = 0;
    int k;
    for (k = 0; k < 4; k++) {
        r |= ((b >> (16 * k)) & 0xFFFF) << (32 * k);
        r |= ((a >> (16 * k)) & 0xFFFF) << (32 * k + 16);
    }
    return r;
}

/* psraw rd, a, sa: each word shifted right by sa, its sign kept. */
static __inline__ vu0_q vu0_psraw(vu0_q a, int sa) {
    vu0_q r = 0;
    int k;
    for (k = 0; k < 4; k++) {
        int w = (int)(a >> (32 * k));
        r |= ((vu0_q)(unsigned int)(w >> sa)) << (32 * k);
    }
    return r;
}

#define pextlh_asm(a, b) vu0_pextlh((vu0_q)(a), (vu0_q)(b))
#define psraw_asm(a, sa) vu0_psraw((vu0_q)(a), (sa))

/* The four rows of a 4x4 matrix, each scaled by `scale` (vmulx.xyzw). */
static __inline__ void mat_scale(void* dst, void* src, float scale) {
    float *d = (float*)dst, *s = (float*)src;
    int k;
    for (k = 0; k < 16; k++) {
        d[k] = s[k] * scale;
    }
}

#endif /* VU0_H */
