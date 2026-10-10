/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The game's hand-written assembly routines that only drive the console's
 * hardware, answered by the port (hostgen.json, "host_functions"). Each is
 * written from what the retail routine does, never from its bytes. */
#include "game_protos.h"
#include "openrac/game_host.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* InitDma: sets the DMA controller's priority bit, enables it and clears the
 * tag address of the VIF0, VIF1, GIF, IPU and SPR channels. The port has no
 * DMA controller: the game's display lists are read at sceDmaSend. */
void func_0020C268(void) {}

/* The square root and the reciprocal square root the hand-written vector
 * routines (hand/) use, as the FPU's sqrt.s and rsqrt.s; translated code calls
 * a named function of its own under the game_ prefix. */
float game_openrac_sqrt(float x) { return sqrtf(x); }
float game_openrac_rsqrt(float a, float b) { return a / sqrtf(b); }

/* A trap the game raises on purpose (teq $0, $0 in a hand-written routine): an assertion of the
 * game that failed. The console would stop in its exception handler; the port says so and stops. */
void game_openrac_game_trap(void) {
    fprintf(stderr, "[error] the game raised a trap (a failed assertion of its own); stopping\n");
    exit(3);
}

/* VU0_loadMicroProgram (func_002347F0, matched C in the decompilation): waits for VIF0, points it
 * at a microprogram and starts the transfer, then waits again. There is no vector unit 0 in the
 * port, and VIF0's status register is plain memory there, so the game's wait would never end: the
 * routines that ran these programs are the port's own C (hand/). */
void func_002347F0(gaddr program) {
    (void)program;
}

/* The asset readers' WAD decompressor (game/common/lib/wad.cpp). */
uint32_t openrac_lib_wad_decompress(const uint8_t *src, uint8_t *dst, uint32_t capacity);

/* WadDecompress (func_0020C468, hand-written): decompresses the WAD stream at src into dst. The
 * retail routine streams through the scratchpad with its DMA channel and returns the size it wrote,
 * which the game's declarations of it do not read; the port decompresses straight from game
 * memory. Its output may run to the end of main memory. */
void func_0020C468(int src, int dst) {
    const uint32_t end_of_ram = 0x02000000u;
    const gaddr to = (gaddr)dst;
    openrac_lib_wad_decompress((const uint8_t *)G(src), (uint8_t *)G(to),
                               to < end_of_ram ? end_of_ram - to : 0);
}

/*
 * FastCos and FastSin. The retail routines hand the angle to a VU0 microprogram (the patch
 * InitOnce uploads from D_0010E4C0 to VU0 address 0xC80): the entry at 0xC80 adds pi/2 and goes on
 * into the sine at 0xC90, which folds the angle into [-pi/2, pi/2] (max(min(x, pi - x), -pi - x))
 * and sums x + c0 x^3 + c1 x^5 + c2 x^7 + c3 x^9 in that order. (ReRAC's moby_light.rs describes
 * the same microprogram, as a reference.) The constants are the microprogram's own: they are read
 * from its I-register immediates in the game's memory the first time, so nothing of the game is
 * copied here; the Taylor terms stand in only if they cannot be found.
 */


/* A word of the VU0 patch microprogram in the game's memory (D_0010E4C0). */
static unsigned word(int i) {
    unsigned w;
    memcpy(&w, G(0x0010E4C0u + (gaddr)i * 4u), 4);
    return w;
}

static float as_float(unsigned w) {
    float f;
    memcpy(&f, &w, 4);
    return f;
}

static float sincos_half_pi = 1.57079625f;
static float sincos_pi = 3.1415925f;
static float sincos_c[4] = {-1.0f / 6.0f, 1.0f / 120.0f, -1.0f / 5040.0f, 1.0f / 362880.0f};
static int sincos_ready;

/* True when `v` lies within a tenth of a percent of `want`. */
static int sincos_near(float v, float want) {
    float d = v - want;
    float tolerance = want * 0.001f;
    if (tolerance < 0.0f) {
        tolerance = -tolerance;
    }
    return d <= tolerance && d >= -tolerance;
}

static void sincos_init(void) {
    int i;
    int found = 0;

    sincos_ready = 1;
    /* The program follows its MPG code (0x4A, 0x5E instructions, at 0x190); the immediates are the
       lower words of the pairs whose upper word has the I bit (bit 31) set. */
    for (i = 0; i < 64; i++) {
        if (word(i) == 0x4A5E0190u) {
            break;
        }
    }
    if (i == 64) {
        return;
    }
    i++;
    if (i & 1) {
        i++;
    }
    for (; i + 1 < 64 + 0x5E * 2; i += 2) {
        unsigned upper = word(i + 1);
        float v;

        if ((upper & 0x80000000u) == 0) {
            continue;
        }
        v = as_float(word(i));
        if (sincos_near(v, 1.5707963f)) {
            sincos_half_pi = v;
            found |= 1;
        } else if (sincos_near(v, 3.1415926f)) {
            sincos_pi = v;
            found |= 2;
        } else if (sincos_near(v, -1.0f / 6.0f)) {
            sincos_c[0] = v;
            found |= 4;
        } else if (sincos_near(v, 1.0f / 120.0f)) {
            sincos_c[1] = v;
            found |= 8;
        } else if (sincos_near(v, -1.0f / 5040.0f) || (v < -1.9e-4f && v > -2.0e-4f)) {
            sincos_c[2] = v;
            found |= 16;
        } else if (v > 2.5e-6f && v < 2.9e-6f) {
            sincos_c[3] = v;
            found |= 32;
        }
    }
    (void)found;
}

/* The microprogram's sine of an angle in radians (entry 0xC90). */
static float vu0_sine(float a) {
    float x, folded, x2, x3, x5, x7, x9, sum;

    if (!sincos_ready) {
        sincos_init();
    }
    /* min(x, pi - x), then max with -pi - x */
    folded = sincos_pi - a;
    x = a < folded ? a : folded;
    folded = -sincos_pi - a;
    x = x > folded ? x : folded;
    x2 = x * x;
    x3 = x * x2;
    x5 = x3 * x2;
    x7 = x5 * x2;
    x9 = x7 * x2;
    sum = x;
    sum = sum + x3 * sincos_c[0];
    sum = sum + x5 * sincos_c[1];
    sum = sum + x7 * sincos_c[2];
    sum = sum + x9 * sincos_c[3];
    return sum;
}

/* FastCos(a): the sine of a + pi/2 (entry 0xC80). */
float func_001F9F90(float a) {
    if (!sincos_ready) {
        sincos_init();
    }
    return vu0_sine(a + sincos_half_pi);
}

/* FastSin(a) (entry 0xC90). */
float func_001F9FA8(float a) {
    return vu0_sine(a);
}

/*
 * The game's movie player (logos, cut scenes). Not played yet: it returns at once with 0, as if
 * the movie ended, until the port's own MPEG player (media/) is connected here.
 */
int func_0023B670(int a0, int a1, int a2, gaddr a3, int a4) {
    (void)a0;
    (void)a1;
    (void)a2;
    (void)a3;
    (void)a4;
    fprintf(stderr, "[info] movie skipped (the port does not play movies yet)\n");
    return 0;
}

/*
 * The VIF1 DMA interrupt handler (hand-written assembly, installed by DMAC_VIF1_Enable). On the
 * console a display list's fence tags interrupt here and each clears its bit in the fence word at
 * D_00160FE0 that the game waits on; the end of the list clears all five. The port sends no list to
 * hardware (the renderer takes it as it is), so every transfer has ended at once.
 */
void func_00235118(void) {
    uint32_t fences;
    memcpy(&fences, G(0x00160FE0u), 4);
    fences &= ~0x1Fu;
    memcpy(G(0x00160FE0u), &fences, 4);
}
