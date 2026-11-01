/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

float D_L01_001CAD00[9] __asm__("D_L01_001CAD00"); /* as the kernel names it: no underscore of Mach-O's */
void FUN_L01_00262038(char*, int, int, char*);
#define CHECK(c)                                                                                   \
    do {                                                                                           \
        if (!(c)) {                                                                                \
            fprintf(stderr, "%d: %s\n", __LINE__, #c);                                             \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)

int main(void) {
    char *tile = calloc(1, 0x1190), *scratch = calloc(1, 0x800);
    CHECK(tile && scratch);
    memset(tile + 0xC, 0xFF, 8);  // No neighbouring tiles in this fixture.
    float* out = (float*)(tile + 0x50);
    D_L01_001CAD00[7] = .16f;
    /* Both retail write paths, including a value whose damped result crosses
     * below the threshold: the branch must test the undamped difference. */
    const float differences[] = {.35f, -.35f, .001f, -.001f, .16f, -.16f, .161f, -.161f};
    for (unsigned n = 0; n < sizeof(differences) / sizeof(differences[0]); ++n) {
        out[0] = -differences[n];
        FUN_L01_00262038(scratch, 0, 0, tile);
        const float expected = fabsf(differences[n]) < .16f
                                   ? differences[n]
                                   : differences[n] - differences[n] * .0625f;
        CHECK(fabsf(out[0] - expected) < 1e-7f);
    }
    /* With a zero threshold and zero neighbours, each history receives exactly
     * 120 updates. This checks stale-buffer removal and the original 1/16 damping. */
    memset(tile + 0x50, 0, 3 * 0x5C0);
    ((float*)(tile + 0x50))[0] = .35f;
    D_L01_001CAD00[7] = 0;
    for (unsigned tick = 0; tick < 240; ++tick) {
        FUN_L01_00262038(scratch, 0, tick % 2, tile);
    }
    const float expected = .35f * powf(1.f - .0625f, 120.f);
    CHECK(fabsf(((float*)(tile + 0x50))[0] - expected) < 1e-6f);
    CHECK(((float*)(tile + 0x50 + 0x5C0))[0] == 0.f);
    /* Keep the grid metadata and neighbouring buffer boundaries intact. */
    for (unsigned i = 0xC; i < 0x14; ++i) {
        CHECK((unsigned char)tile[i] == 0xFF);
    }
    free(tile);
    free(scratch);
    return 0;
}
