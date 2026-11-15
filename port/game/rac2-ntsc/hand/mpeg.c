/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/* Going Commando's libmpeg routines that the decompilation keeps as hand-written assembly (ASM_FUNC in
 * src/core/00129450.c), in C: the software motion compensation that
 * builds a macroblock's prediction with the EE's multimedia instructions and the empty hook beside it
 * (mpeg_spr.c and mpeg_csc.c have the DMA routines). Each was written from what the routine's
 * instructions do and in their order (loads before the stores that follow them, each field of the
 * argument read when the routine reads it), so that overlapping buffers come out as on the console;
 * handdiff.py checks each against hostgen's translation of the retail routine.
 *
 * In v1.01's names: hostgen renames this file for PAL and USA v2.00 ("primary_hand"). */
#include "common.h"

/* What $v0 holds at the return. The decoder calls these through a table and does not read it, but
 * the routines leave a value there that the port's check compares: the first two halfwords of the
 * last row stored for those that build the row in $v0's register (0 when no row is stored, as the
 * port starts its registers), the prediction's row step for the chroma ones that keep that in it,
 * and 0 for the rest, which leave $v0 as their caller had it (the port's registers start at 0). */

/* The motion compensation's argument: where the prediction goes (halfwords, two bytes per pixel,
 * rows twice the source stride apart), the byte offset of the block inside the source rows, the
 * number of rows of the first part and of the second (a field picture's other half, past a gap of
 * 0x80 bytes for luma and 0x140 for chroma), the source row stride, and the two sources: the 16
 * bytes (8 for chroma) at src1 continue at src2. Luma blocks are 16 pixels wide; chroma blocks are 8
 * wide and done twice, Cb and then Cr (0x40 further in the sources, 0x80 in the prediction). */
typedef struct {
    u32 dst;
    s32 shift;
    s32 rows;
    s32 rows2;
    s32 stride;
    u32 src1;
    u32 src2;
} McArg;

/* lq: the 16 bytes of the quadword s1 lies in, then those of s2's (the low four address bits are
 * ignored). */
static void mc_load16(u8 *c, u32 s1, u32 s2)
{
    int i;
    s1 &= ~15u;
    s2 &= ~15u;
    for (i = 0; i < 16; i++) {
        c[i] = *(u8 *)(s1 + i);
    }
    for (i = 0; i < 16; i++) {
        c[16 + i] = *(u8 *)(s2 + i);
    }
}

/* ld of both sources (low three address bits ignored), joined into 16 bytes (pcpyld) and turned
 * right by the block's offset (qfsrv of the register with itself): c[i] is byte (shift + i) & 15. */
static void mc_load8(u8 *c, u32 s1, u32 s2, int sh)
{
    u8 t[16];
    int i;
    s1 &= ~7u;
    s2 &= ~7u;
    for (i = 0; i < 8; i++) {
        t[i] = *(u8 *)(s1 + i);
    }
    for (i = 0; i < 8; i++) {
        t[8 + i] = *(u8 *)(s2 + i);
    }
    for (i = 0; i < 16; i++) {
        c[i] = t[(sh + i) & 15];
    }
}

/* A row of n halfwords from the prediction (lq) and to it (sq). */
static void mc_get(u16 *v, u32 d, int n)
{
    int i;
    d &= ~15u;
    for (i = 0; i < n; i++) {
        v[i] = *(u16 *)(d + i * 2);
    }
}

static void mc_put(u32 d, u16 *v, int n)
{
    int i;
    d &= ~15u;
    for (i = 0; i < n; i++) {
        *(u16 *)(d + i * 2) = v[i];
    }
}

/* The halfword lanes' rounding: (x + k) >> s, logical, with the add wrapping at 16 bits (paddh,
 * psrlh); and the averaging with the prediction that adds 1 only to a lane that is above zero as a
 * signed halfword (pcgth, psrlh 15, paddh, psrlh 1). */
static u16 mc_round(u16 x, u16 k, int s)
{
    return (u16)(x + k) >> s;
}

static u16 mc_half_pos(u16 x)
{
    return (u16)(x + ((s16)x > 0 ? 1 : 0)) >> 1;
}

/* Luma, copy: each row's 16 bytes widened to halfwords. */
int func_0012A3E8(McArg *a)
{
    u32 s1 = a->src1, s2 = a->src2;
    s32 n = a->rows;
    u32 d = a->dst;
    int sh = a->shift & 15;
    s32 st = a->stride;
    s32 first = -1, t;
    u8 c[32];
    u16 v[16];
    int i;

loop:
    mc_load16(c, s1, s2);
    n--;
    s1 += st;
    for (i = 0; i < 16; i++) {
        v[i] = c[sh + i];
    }
    mc_put(d, v, 8);
    s2 += st;
    mc_put(d + 16, v + 8, 8);
    d += st * 2;
    if (n > 0) {
        goto loop;
    }
    s1 += 0x80;
    s2 += 0x80;
    n = a->rows2;
    t = first & n;
    first = 0;
    if (t != 0) {
        goto loop;
    }
    return 0;
}

/* Chroma, copy: each row's 8 bytes widened, for Cb then Cr. */
int func_0012A460(McArg *a)
{
    u32 s1 = a->src1, s2 = a->src2;
    u32 d = a->dst;
    int sh = a->shift & 15;
    s32 st = a->stride;
    s32 outer = -1, first, n, t;
    u8 c[16];
    u16 v[8];
    int i;

pass:
    n = a->rows;
    first = -1;
loop:
    mc_load8(c, s1, s2, sh);
    for (i = 0; i < 8; i++) {
        v[i] = c[i];
    }
    mc_put(d, v, 8);
    n--;
    s1 += st;
    d += st * 2;
    s2 += st;
    if (n > 0) {
        goto loop;
    }
    s1 += 0x140;
    s2 += 0x140;
    n = a->rows2;
    t = first & n;
    first = 0;
    if (t != 0) {
        goto loop;
    }
    s1 = a->src1 + 0x40;
    s2 = a->src2 + 0x40;
    d = a->dst + 0x80;
    t = outer;
    outer = 0;
    if (t != 0) {
        goto pass;
    }
    return 0;
}

/* Luma, half a pixel down: each row the rounded mean of it and the row above. */
int func_0012A4F8(McArg *a)
{
    u32 s1 = a->src1, s2 = a->src2;
    s32 n = a->rows;
    u32 d = a->dst;
    int sh = a->shift & 15;
    s32 st = a->stride;
    s32 first = -1, t;
    u8 c[32];
    u16 prev[16], cur[16], v[16];
    int i;
    int ret = 0;

    mc_load16(c, s1, s2);
    for (i = 0; i < 16; i++) {
        prev[i] = c[sh + i];
    }
    if (n == 0) {
        goto next;
    }
loop:
    s1 += st;
    s2 += st;
    mc_load16(c, s1, s2);
    for (i = 0; i < 16; i++) {
        cur[i] = c[sh + i];
    }
    n--;
    for (i = 0; i < 16; i++) {
        v[i] = mc_round((u16)(prev[i] + cur[i]), 1, 1);
        prev[i] = cur[i];
    }
    mc_put(d, v, 8);
    ret = v[0] | (v[1] << 16);
    mc_put(d + 16, v + 8, 8);
    d += st * 2;
    if (n > 0) {
        goto loop;
    }
next:
    s1 += 0x80;
    s2 += 0x80;
    n = a->rows2;
    t = first & n;
    first = 0;
    if (t != 0) {
        goto loop;
    }
    return ret;
}

/* Chroma, half a pixel down. The second part is taken when the low 16 bits of its row count are not
 * zero: the routine keeps its two loop flags in one halfword (psrah). */
int func_0012A5B0(McArg *a)
{
    u32 s1 = a->src1, s2 = a->src2;
    u32 d = a->dst;
    int sh = a->shift & 15;
    s32 st = a->stride;
    u32 flags = 1, t;
    s32 n;
    u8 c[16];
    u16 prev[8], cur[8], v[8];
    int i;

pass:
    n = a->rows;
    mc_load8(c, s1, s2, sh);
    flags |= 0x8000;
    for (i = 0; i < 8; i++) {
        prev[i] = c[i];
    }
    if (n == 0) {
        goto next;
    }
loop:
    s1 += st;
    s2 += st;
    mc_load8(c, s1, s2, sh);
    for (i = 0; i < 8; i++) {
        cur[i] = c[i];
    }
    n--;
    for (i = 0; i < 8; i++) {
        v[i] = mc_round((u16)(cur[i] + prev[i]), 1, 1);
        prev[i] = cur[i];
    }
    mc_put(d, v, 8);
    d += st * 2;
    if (n > 0) {
        goto loop;
    }
next:
    t = (flags & 0x8000) ? 0xFFFF : 0;
    s1 += 0x140;
    n = a->rows2;
    s2 += 0x140;
    t &= n;
    flags &= 0x7FFF;
    if (t != 0) {
        goto loop;
    }
    s1 = a->src1 + 0x40;
    s2 = a->src2 + 0x40;
    d = a->dst + 0x80;
    t = flags & 1;
    flags &= 0xFFFE;
    if (t != 0) {
        goto pass;
    }
    return 0;
}

/* Luma, half a pixel right: the rounded mean of each pixel and the next. */
int func_0012A680(McArg *a)
{
    u32 s1 = a->src1, s2 = a->src2;
    s32 n = a->rows;
    u32 d = a->dst;
    int sh = a->shift & 15;
    s32 st = a->stride;
    s32 first = -1, t;
    u8 c[32];
    u16 v[16];
    int i;
    int ret = 0;

loop:
    mc_load16(c, s1, s2);
    n--;
    for (i = 0; i < 16; i++) {
        v[i] = mc_round((u16)(c[sh + i] + c[sh + 1 + i]), 1, 1);
    }
    mc_put(d, v, 8);
    ret = v[0] | (v[1] << 16);
    mc_put(d + 16, v + 8, 8);
    s1 += st;
    s2 += st;
    d += st * 2;
    if (n > 0) {
        goto loop;
    }
    s1 += 0x80;
    s2 += 0x80;
    n = a->rows2;
    t = first & n;
    first = 0;
    if (t != 0) {
        goto loop;
    }
    return ret;
}

/* Chroma, half a pixel right. */
int func_0012A730(McArg *a)
{
    u32 s1 = a->src1, s2 = a->src2;
    u32 d = a->dst;
    int sh = a->shift & 15;
    s32 st = a->stride;
    s32 outer = -1, first, n, t;
    u8 c[16];
    u16 v[8];
    int i;

pass:
    n = a->rows;
    first = -1;
loop:
    mc_load8(c, s1, s2, sh);
    n--;
    s1 += st;
    s2 += st;
    for (i = 0; i < 8; i++) {
        v[i] = mc_round((u16)(c[i] + c[i + 1]), 1, 1);
    }
    mc_put(d, v, 8);
    d += st * 2;
    if (n > 0) {
        goto loop;
    }
    s1 += 0x140;
    s2 += 0x140;
    n = a->rows2;
    t = first & n;
    first = 0;
    if (t != 0) {
        goto loop;
    }
    s1 = a->src1 + 0x40;
    s2 = a->src2 + 0x40;
    d = a->dst + 0x80;
    t = outer;
    outer = 0;
    if (t != 0) {
        goto pass;
    }
    return (int)((u32)st << 1);
}

/* Luma, half a pixel right and down: the rounded mean of four. */
int func_0012A7E8(McArg *a)
{
    u32 s1 = a->src1, s2 = a->src2;
    s32 n = a->rows;
    u32 d = a->dst;
    int sh = a->shift & 15;
    s32 st = a->stride;
    s32 first = -1, t;
    u8 c[32];
    u16 prev[16], cur[16], v[16];
    int i;
    int ret = 0;

    mc_load16(c, s1, s2);
    for (i = 0; i < 16; i++) {
        prev[i] = (u16)(c[sh + i] + c[sh + 1 + i]);
    }
    if (n == 0) {
        goto next;
    }
loop:
    s1 += st;
    s2 += st;
    mc_load16(c, s1, s2);
    n--;
    for (i = 0; i < 16; i++) {
        cur[i] = (u16)(c[sh + i] + c[sh + 1 + i]);
        v[i] = mc_round((u16)(prev[i] + cur[i]), 2, 2);
        prev[i] = cur[i];
    }
    mc_put(d, v, 8);
    ret = v[0] | (v[1] << 16);
    mc_put(d + 16, v + 8, 8);
    d += st * 2;
    if (n > 0) {
        goto loop;
    }
next:
    s1 += 0x80;
    s2 += 0x80;
    n = a->rows2;
    t = first & n;
    first = 0;
    if (t != 0) {
        goto loop;
    }
    return ret;
}

/* Chroma, half a pixel right and down. The routine moves src1 on a row before src2 (the first row
 * reads both, then src1 steps; each later row steps src2, reads both, steps src1), so after a part
 * src1 is a row ahead: the second part reads src1 one row further down than src2. */
int func_0012A8E0(McArg *a)
{
    u32 s1 = a->src1, s2 = a->src2;
    u32 d = a->dst;
    int sh = a->shift & 15;
    s32 st = a->stride;
    u32 flags = 1, t;
    s32 n;
    u8 c[16];
    u16 prev[8], cur[8], v[8];
    int i;

pass:
    n = a->rows;
    mc_load8(c, s1, s2, sh);
    s1 += st;
    flags |= 0x8000;
    for (i = 0; i < 8; i++) {
        prev[i] = (u16)(c[i] + c[i + 1]);
    }
    if (n == 0) {
        goto next;
    }
loop:
    s2 += st;
    mc_load8(c, s1, s2, sh);
    n--;
    s1 += st;
    for (i = 0; i < 8; i++) {
        cur[i] = (u16)(c[i] + c[i + 1]);
        v[i] = mc_round((u16)(cur[i] + prev[i]), 2, 2);
        prev[i] = cur[i];
    }
    mc_put(d, v, 8);
    d += st * 2;
    if (n > 0) {
        goto loop;
    }
next:
    t = (flags & 0x8000) ? 0xFFFF : 0;
    s1 += 0x140;
    n = a->rows2;
    s2 += 0x140;
    t &= n;
    flags &= 0x7FFF;
    if (t != 0) {
        goto loop;
    }
    s1 = a->src1 + 0x40;
    s2 = a->src2 + 0x40;
    d = a->dst + 0x80;
    t = flags & 1;
    flags &= 0xFFFE;
    if (t != 0) {
        goto pass;
    }
    return 0;
}

/* Luma, copy, averaged with the prediction already there (a B block's second direction). */
int func_0012A9E0(McArg *a)
{
    u32 s1 = a->src1, s2 = a->src2;
    s32 n = a->rows;
    u32 d = a->dst;
    int sh = a->shift & 15;
    s32 st = a->stride;
    s32 first = -1, t;
    u8 c[32];
    u16 p[16], v[16];
    int i;
    int ret = 0;

loop:
    mc_load16(c, s1, s2);
    mc_get(p, d, 8);
    mc_get(p + 8, d + 16, 8);
    for (i = 0; i < 16; i++) {
        v[i] = mc_round((u16)(p[i] + c[sh + i]), 1, 1);
    }
    mc_put(d, v, 8);
    ret = v[0] | (v[1] << 16);
    mc_put(d + 16, v + 8, 8);
    n--;
    s1 += st;
    d += st * 2;
    s2 += st;
    if (n > 0) {
        goto loop;
    }
    s1 += 0x80;
    s2 += 0x80;
    n = a->rows2;
    t = first & n;
    first = 0;
    if (t != 0) {
        goto loop;
    }
    return ret;
}

/* Chroma, copy, averaged with the prediction. */
int func_0012AA80(McArg *a)
{
    u32 s1 = a->src1, s2 = a->src2;
    u32 d = a->dst;
    int sh = a->shift & 15;
    s32 st = a->stride;
    s32 outer = -1, first, n, t;
    u8 c[16];
    u16 p[8], v[8];
    int i;

pass:
    n = a->rows;
    first = -1;
loop:
    mc_load8(c, s1, s2, sh);
    n--;
    s1 += st;
    s2 += st;
    mc_get(p, d, 8);
    for (i = 0; i < 8; i++) {
        v[i] = mc_half_pos((u16)(c[i] + p[i]));
    }
    mc_put(d, v, 8);
    d += st * 2;
    if (n > 0) {
        goto loop;
    }
    s1 += 0x140;
    s2 += 0x140;
    n = a->rows2;
    t = first & n;
    first = 0;
    if (t != 0) {
        goto loop;
    }
    s1 = a->src1 + 0x40;
    s2 = a->src2 + 0x40;
    d = a->dst + 0x80;
    t = outer;
    outer = 0;
    if (t != 0) {
        goto pass;
    }
    return (int)((u32)st << 1);
}

/* Luma, half a pixel down, averaged with the prediction. The routine averages the left eight
 * pixels with +1 and the right eight with +1 only where the sum is above zero. */
int func_0012AB30(McArg *a)
{
    u32 s1 = a->src1, s2 = a->src2;
    s32 n = a->rows;
    u32 d = a->dst;
    int sh = a->shift & 15;
    s32 st = a->stride;
    s32 first = -1, t;
    u8 c[32];
    u16 prev[16], cur[16], m[16], p[16], v[16];
    int i;
    int ret = 0;

    mc_load16(c, s1, s2);
    for (i = 0; i < 16; i++) {
        prev[i] = c[sh + i];
    }
    if (n == 0) {
        goto next;
    }
loop:
    s1 += st;
    s2 += st;
    mc_load16(c, s1, s2);
    for (i = 0; i < 16; i++) {
        cur[i] = c[sh + i];
    }
    n--;
    for (i = 0; i < 16; i++) {
        m[i] = mc_round((u16)(prev[i] + cur[i]), 1, 1);
        prev[i] = cur[i];
    }
    mc_get(p, d, 8);
    mc_get(p + 8, d + 16, 8);
    for (i = 0; i < 8; i++) {
        v[i] = mc_round((u16)(m[i] + p[i]), 1, 1);
    }
    for (i = 8; i < 16; i++) {
        v[i] = mc_half_pos((u16)(m[i] + p[i]));
    }
    mc_put(d, v, 8);
    ret = v[0] | (v[1] << 16);
    mc_put(d + 16, v + 8, 8);
    d += st * 2;
    if (n > 0) {
        goto loop;
    }
next:
    s1 += 0x80;
    s2 += 0x80;
    n = a->rows2;
    t = first & n;
    first = 0;
    if (t != 0) {
        goto loop;
    }
    return ret;
}

/* Chroma, half a pixel down, averaged with the prediction. */
int func_0012AC10(McArg *a)
{
    u32 s1 = a->src1, s2 = a->src2;
    u32 d = a->dst;
    int sh = a->shift & 15;
    s32 st = a->stride;
    u32 flags = 1, t;
    s32 n;
    u8 c[16];
    u16 prev[8], cur[8], m[8], p[8], v[8];
    int i;

pass:
    n = a->rows;
    mc_load8(c, s1, s2, sh);
    flags |= 0x8000;
    for (i = 0; i < 8; i++) {
        prev[i] = c[i];
    }
    if (n == 0) {
        goto next;
    }
loop:
    s1 += st;
    s2 += st;
    mc_load8(c, s1, s2, sh);
    for (i = 0; i < 8; i++) {
        cur[i] = c[i];
    }
    n--;
    for (i = 0; i < 8; i++) {
        m[i] = mc_round((u16)(cur[i] + prev[i]), 1, 1);
        prev[i] = cur[i];
    }
    mc_get(p, d, 8);
    for (i = 0; i < 8; i++) {
        v[i] = mc_half_pos((u16)(m[i] + p[i]));
    }
    mc_put(d, v, 8);
    d += st * 2;
    if (n > 0) {
        goto loop;
    }
next:
    t = (flags & 0x8000) ? 0xFFFF : 0;
    s1 += 0x140;
    n = a->rows2;
    s2 += 0x140;
    t &= n;
    flags &= 0x7FFF;
    if (t != 0) {
        goto loop;
    }
    s1 = a->src1 + 0x40;
    s2 = a->src2 + 0x40;
    d = a->dst + 0x80;
    t = flags & 1;
    flags &= 0xFFFE;
    if (t != 0) {
        goto pass;
    }
    return 0;
}

/* Luma, half a pixel right, averaged with the prediction (left eight +1, right eight +1 above
 * zero, as func_0012AB30). */
int func_0012ACF8(McArg *a)
{
    u32 s1 = a->src1, s2 = a->src2;
    s32 n = a->rows;
    u32 d = a->dst;
    int sh = a->shift & 15;
    s32 st = a->stride;
    s32 first = -1, t;
    u8 c[32];
    u16 m[16], p[16], v[16];
    int i;
    int ret = 0;

loop:
    mc_load16(c, s1, s2);
    n--;
    for (i = 0; i < 16; i++) {
        m[i] = mc_round((u16)(c[sh + i] + c[sh + 1 + i]), 1, 1);
    }
    mc_get(p, d, 8);
    mc_get(p + 8, d + 16, 8);
    for (i = 0; i < 8; i++) {
        v[i] = mc_round((u16)(m[i] + p[i]), 1, 1);
    }
    for (i = 8; i < 16; i++) {
        v[i] = mc_half_pos((u16)(m[i] + p[i]));
    }
    mc_put(d, v, 8);
    ret = v[0] | (v[1] << 16);
    mc_put(d + 16, v + 8, 8);
    s1 += st;
    s2 += st;
    d += st * 2;
    if (n > 0) {
        goto loop;
    }
    s1 += 0x80;
    s2 += 0x80;
    n = a->rows2;
    t = first & n;
    first = 0;
    if (t != 0) {
        goto loop;
    }
    return ret;
}

/* Chroma, half a pixel right, averaged with the prediction. */
int func_0012ADD0(McArg *a)
{
    u32 s1 = a->src1, s2 = a->src2;
    u32 d = a->dst;
    int sh = a->shift & 15;
    s32 st = a->stride;
    s32 outer = -1, first, n, t;
    u8 c[16];
    u16 m[8], p[8], v[8];
    int i;

pass:
    n = a->rows;
    first = -1;
loop:
    mc_load8(c, s1, s2, sh);
    n--;
    s1 += st;
    s2 += st;
    for (i = 0; i < 8; i++) {
        m[i] = mc_round((u16)(c[i] + c[i + 1]), 1, 1);
    }
    mc_get(p, d, 8);
    for (i = 0; i < 8; i++) {
        v[i] = mc_half_pos((u16)(m[i] + p[i]));
    }
    mc_put(d, v, 8);
    d += st * 2;
    if (n > 0) {
        goto loop;
    }
    s1 += 0x140;
    s2 += 0x140;
    n = a->rows2;
    t = first & n;
    first = 0;
    if (t != 0) {
        goto loop;
    }
    s1 = a->src1 + 0x40;
    s2 = a->src2 + 0x40;
    d = a->dst + 0x80;
    t = outer;
    outer = 0;
    if (t != 0) {
        goto pass;
    }
    return (int)((u32)st << 1);
}

/* Luma, half a pixel right and down, averaged with the prediction. The rounding constant of the
 * four-pixel mean is 2 for the first row only: the routine reuses that register for the 1 of the
 * averaging and does not set it back, so every later row (and the second part) adds 1 before the
 * shift by two. */
int func_0012AEA0(McArg *a)
{
    u32 s1 = a->src1, s2 = a->src2;
    s32 n = a->rows;
    u32 d = a->dst;
    int sh = a->shift & 15;
    s32 st = a->stride;
    s32 first = -1, t;
    u16 k = 2;
    u8 c[32];
    u16 prev[16], cur[16], m[16], p[16], v[16];
    int i;
    int ret = 0;

    mc_load16(c, s1, s2);
    for (i = 0; i < 16; i++) {
        prev[i] = (u16)(c[sh + i] + c[sh + 1 + i]);
    }
    if (n == 0) {
        goto next;
    }
loop:
    s1 += st;
    s2 += st;
    mc_load16(c, s1, s2);
    n--;
    for (i = 0; i < 16; i++) {
        cur[i] = (u16)(c[sh + i] + c[sh + 1 + i]);
        m[i] = mc_round((u16)(prev[i] + cur[i]), k, 2);
        prev[i] = cur[i];
    }
    mc_get(p, d, 8);
    mc_get(p + 8, d + 16, 8);
    k = 1;
    for (i = 0; i < 16; i++) {
        v[i] = mc_round((u16)(m[i] + p[i]), k, 1);
    }
    mc_put(d, v, 8);
    ret = v[0] | (v[1] << 16);
    mc_put(d + 16, v + 8, 8);
    d += st * 2;
    if (n > 0) {
        goto loop;
    }
next:
    s1 += 0x80;
    s2 += 0x80;
    n = a->rows2;
    t = first & n;
    first = 0;
    if (t != 0) {
        goto loop;
    }
    return ret;
}

/* Chroma, half a pixel right and down, averaged with the prediction (src1 a row ahead in the
 * second part, as func_0012A8E0). */
int func_0012AFC0(McArg *a)
{
    u32 s1 = a->src1, s2 = a->src2;
    u32 d = a->dst;
    int sh = a->shift & 15;
    s32 st = a->stride;
    u32 flags = 1, t;
    s32 n;
    u8 c[16];
    u16 prev[8], cur[8], m[8], p[8], v[8];
    int i;

pass:
    n = a->rows;
    mc_load8(c, s1, s2, sh);
    s1 += st;
    flags |= 0x8000;
    for (i = 0; i < 8; i++) {
        prev[i] = (u16)(c[i] + c[i + 1]);
    }
    if (n == 0) {
        goto next;
    }
loop:
    s2 += st;
    mc_load8(c, s1, s2, sh);
    n--;
    s1 += st;
    for (i = 0; i < 8; i++) {
        cur[i] = (u16)(c[i] + c[i + 1]);
        m[i] = mc_round((u16)(cur[i] + prev[i]), 2, 2);
        prev[i] = cur[i];
    }
    mc_get(p, d, 8);
    for (i = 0; i < 8; i++) {
        v[i] = mc_half_pos((u16)(m[i] + p[i]));
    }
    mc_put(d, v, 8);
    d += st * 2;
    if (n > 0) {
        goto loop;
    }
next:
    t = (flags & 0x8000) ? 0xFFFF : 0;
    s1 += 0x140;
    n = a->rows2;
    s2 += 0x140;
    t &= n;
    flags &= 0x7FFF;
    if (t != 0) {
        goto loop;
    }
    s1 = a->src1 + 0x40;
    s2 = a->src2 + 0x40;
    d = a->dst + 0x80;
    t = flags & 1;
    flags &= 0xFFFE;
    if (t != 0) {
        goto pass;
    }
    return 0;
}

/* An empty routine (jr $ra) the decoder calls as a hook; $v0 as for the rest above. */
int func_0012B180(void)
{
    return 0;
}
