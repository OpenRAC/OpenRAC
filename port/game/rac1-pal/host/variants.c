/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Ratchet & Clank (PAL): functions the decompilation's catalogue (config/overlays/functions.tsv)
 * folds into another by its fingerprint, although their code differs from it in a constant or a
 * field offset (split_places.tsv). Each is the function it was folded into with the words that
 * differ, read from the retail copies; the places are theirs alone (hostgen, read_split_places).
 */
#include "game_protos.h"
#include "openrac/game_host.h"

#include <stdint.h>

/* The hero block, D_0013E633 + 0xE1D. */
#define HERO 0x0013F450u

/* func_L00_00211F68's other: clears the hero's +0x227C and +0x2278 (not the target speed and
 * speed). Ratchet's animation advance (func_L00_00232EF0) starts with it. */
void func_L00_00232A00(void) {
    GREF(int, HERO + 0x227C) = 0;
    GREF(int, HERO + 0x2278) = 0;
}

/* func_L00_0020A858's three neighbours: the same two-float setter on D_L00_0017A780, at
 * +0x154/+0x158, +0x204/+0x208 and +0x2B4/+0x2B8 instead of +0xA4/+0xA8. */
void func_L00_0020A870(float a, float b) {
    const gaddr p = OPENRAC_LDATA(0, 0x0017A780u);
    GREF(float, p + 0x154) = a;
    GREF(float, p + 0x158) = b;
}

void func_L00_0020A888(float a, float b) {
    const gaddr p = OPENRAC_LDATA(0, 0x0017A780u);
    GREF(float, p + 0x204) = a;
    GREF(float, p + 0x208) = b;
}

void func_L00_0020A8A0(float a, float b) {
    const gaddr p = OPENRAC_LDATA(0, 0x0017A780u);
    GREF(float, p + 0x2B4) = a;
    GREF(float, p + 0x2B8) = b;
}

/* func_001EC270's level copy: the dispatch record's second hook (+0x10), not its first (+0x08). */
void func_L00_001EBD58(gaddr arg0) {
    const gaddr rec = OPENRAC_DATA(0x001E8F80u) + (gaddr)GREF(short, arg0 + 0x8C) * 0x14u;
    const gaddr fn = GREF(gaddr, rec + 0x10);
    if (fn != 0) {
        GFN(void (*)(gaddr), fn)(arg0);
    }
}

/* func_001FB848's level copies: the same four-word packet (a DMA call of COUNT quadwords at DATA
 * and its return) with other counts and data. */
static void packet_call(uint32_t count, gaddr data) {
    const gaddr at = GREF(gaddr, OPENRAC_DATA(0x00161000u));
    GREF(uint32_t, at + 0) = 0x30000000u | count;
    GREF(uint32_t, at + 4) = data;
    GREF(uint32_t, at + 8) = 0;
    GREF(uint32_t, at + 12) = 0x50000000u | count;
    GREF(gaddr, OPENRAC_DATA(0x00161000u)) = at + 16;
}

void func_L00_00201300(void) { packet_call(0x29, 0x00151C60u); }
void func_L00_002A2020(void) { packet_call(0x03, OPENRAC_DATA(0x001DF180u)); }
void func_L00_002A2148(void) { packet_call(0x0B, 0x0013D010u); }

/* func_0020CC88's level copy: flags +0x20 and +0x21 of D_0013D5C8, not +0x21 and +0x1F. */
int func_L00_0024F054(void) {
    const gaddr base = 0x0013D5C8u;
    return GREF(uint8_t, base + 0x20) != 0 && GREF(uint8_t, base + 0x21) != 0;
}

/* func_00217860's level copy: the track words at +0x38/+0x3C/+0x3A of D_001517D0, not
 * +0x54/+0x58/+0x56. */
void func_L00_00267080(int arg0, long long arg1) {
    const gaddr p = (gaddr)(int)arg1;
    if (p == 0) {
        return;
    }
    GREF(int, p) = arg0;
    if (arg0 != 0) {
        if (GREF(short, p + 10) == 1) {
            GREF(short, p + 10) = 2;
        }
    } else {
        const gaddr b = 0x001517D0u;
        func_002167C0(GREF(short, b + 0x38), GREF(short, b + 0x3C), GREF(short, b + 0x3A));
    }
}

/* func_00229C08's level copy: words 6 and 5 of D_0018A3B0 decide the shrub textures, not 8 and 7. */
void func_L00_00296CD0(void) {
    const gaddr out = OPENRAC_DATA(0x00161000u);
    const gaddr list = OPENRAC_DATA(0x001604F0u);
    const gaddr p = GREF(gaddr, out);
    GREF(gaddr, out) += 16;
    const gaddr l = GREF(gaddr, list);
    GREF(int, l + 0) = 0x20000000;
    GREF(int, l + 4) = (int)GREF(gaddr, out);
    GREF(int, l + 8) = 0;
    GREF(int, l + 12) = 0;
    const gaddr flags = OPENRAC_DATA(0x0018A3B0u);
    if (GREF(int, flags + 6 * 4) != 0 && GREF(int, flags + 5 * 4) != 0) {
        const int size = func_0022B648(GREF(int, 0x0015EF74u));
        func_00234E80();
        if (GREF(int, OPENRAC_DATA(0x001604F8u)) < size) {
            GREF(int, OPENRAC_DATA(0x001604F8u)) = size;
        }
    }
    const gaddr q = GREF(gaddr, out);
    GREF(int, q + 0) = 0x20000000;
    GREF(int, q + 4) = (int)(GREF(gaddr, list) + 16);
    GREF(int, q + 8) = 0;
    GREF(int, q + 12) = 0;
    GREF(gaddr, out) += 16;
    GREF(int, p + 0) = 0x20000000;
    GREF(int, p + 4) = (int)GREF(gaddr, out);
    GREF(int, p + 8) = 0;
    GREF(int, p + 12) = 0;
}

/* func_L00_0028F210's copy in some levels: the sound slot's +0x80, not +0x84. */
int func_L01_002A2B68(int i, int v) {
    GREF(int, 0x0013E650u + (gaddr)(i * 0x70) + 0x80) = v;
    return 1;
}

/* func_L00_002E9A40's neighbour: +0xE4/+0xE8 of the record, not +0xDC/+0xE0. */
void func_L00_002E9A88(float a, float b) {
    const gaddr p = GREF(gaddr, OPENRAC_LDATA(0, 0x00166F00u));
    if (GREF(short, p + 0x86) == 0) {
        const gaddr q = GREF(gaddr, p + 0x70) + 0x40;
        if (a != 0.0f) {
            GREF(float, q + 0xE4) = a;
        }
        if (b != 0.0f) {
            GREF(float, q + 0xE8) = b;
        }
    }
}
