/* NON_MATCHING func_L00_002E72E8 -- src/overlays/shared/vendor_002E1660.c
 * Best so far: SIZE ours 448 / retail 452, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_002E72E8 (camera state reset): three smoothing calls to func_001EC120 then a run of default stores. B
 *   Learned: the three "cond ? a : b" arguments are if/else with a call in each arm (cross-jumped tail; keeps 0.2f
 *   Left: the post-call store block schedules differently (retail loads D_L00_00161D70 into $f2 after the sw $zero
 */
#include "common.h"
extern short D_L00_0015F044;
extern char *D_L00_001690C0;
extern int D_L00_0015F060;
extern short D_L00_00161DD4;
extern short D_L00_00161DD8;
extern short D_L00_00161D70;
extern short D_L00_00161DAC;
extern short D_L00_00161DB4;
extern short D_L00_00161DB0;
extern short D_L00_00161DEC;
extern float func_001EC120(void *, float, float, float, float, float);
/* resets the camera state blocks to their defaults */
void func_L00_002E72E8(char *m) {
    char *q = *(char **)(m + 0x70) + 0x1A8;
    float f, c;
    char *p, *s16, *s17, *g, *s19, *s20, *r;
    *(float *)(q + 0x14) = *(float *)&D_L00_0015F044;
    *(int *)(q + 0x1C) = 0;
    *(int *)(q + 0x20) = 0;
    *(int *)(q + 0x10) = 0;
    *(int *)(q + 0x24) = 0;
    p = *(char **)(m + 0x70);
    g = D_L00_001690C0;
    s16 = p + 0x130;
    s17 = p + 0x40;
    s20 = g + 0x40;
    s19 = g + 0x130;
    if (*(short *)(s16 + 0x3C))
        *(float *)(s16 + 0x2C) = func_001EC120(p + 0x174, *(float *)(s16 + 0x2C), *(float *)(s16 + 0x40),
            *(float *)(s16 + 0x48), *(float *)&D_L00_00161DD4, *(float *)(q + 0x1C));
    else
        *(float *)(s16 + 0x2C) = func_001EC120(p + 0x174, *(float *)(s16 + 0x2C), *(float *)(s19 + 0x2C),
            *(float *)(s16 + 0x48), *(float *)&D_L00_00161DD4, *(float *)(q + 0x1C));
    if (*(short *)(s16 + 0x3E))
        *(float *)(s16 + 0x30) = func_001EC120(s16 + 0x50, *(float *)(s16 + 0x30), *(float *)(s16 + 0x4C),
            *(float *)(s16 + 0x54), *(float *)&D_L00_00161DD8, 0.0f);
    else
        *(float *)(s16 + 0x30) = func_001EC120(s16 + 0x50, *(float *)(s16 + 0x30), *(float *)(s19 + 0x30),
            *(float *)(s16 + 0x54), *(float *)&D_L00_00161DD8, 0.0f);
    if (*(short *)(s17 + 0xDA))
        *(float *)(s17 + 0xB0) = func_001EC120(s17 + 0xBC, *(float *)(s17 + 0xB0), *(float *)(s17 + 0xB4),
            *(float *)(s17 + 0xB8), 0.2f, 0.0f);
    else
        *(float *)(s17 + 0xB0) = func_001EC120(s17 + 0xBC, *(float *)(s17 + 0xB0), *(float *)(s20 + 0xB0),
            *(float *)(s17 + 0xB8), 0.2f, 0.0f);
    f = *(float *)&D_L00_00161D70;
    c = 0.2f;
    *(short *)(s16 + 0x3C) = 0;
    *(short *)(s16 + 0x3E) = 0;
    *(int *)(s16 + 0x40) = 0;
    *(int *)(s16 + 0x4C) = 0;
    *(float *)(s16 + 0x58) = *(float *)(s19 + 0x2C);
    *(short *)(s17 + 0xDA) = 0;
    *(int *)(s17 + 0xB4) = 0;
    *(short *)(*(char **)(m + 0x70) + 0x10) = 1;
    *(short *)(*(char **)(m + 0x70) + 0x22) = 0;
    *(char *)(s17 + 0xD6) = 0;
    *(float *)(s17 + 0xE4) = f;
    *(float *)(s17 + 0xE8) = c;
    *(float *)(s17 + 0xDC) = f;
    *(float *)(s17 + 0xE0) = c;
    r = *(char **)(m + 0x70) + 0x1D0;
    *(float *)(r + 0x44) = *(float *)&D_L00_00161DAC;
    *(int *)(r + 0x48) = D_L00_0015F060;
    *(float *)(r + 0x3C) = *(float *)&D_L00_00161DB4;
    *(float *)(r + 0x40) = *(float *)&D_L00_00161DB0;
    r = *(char **)(m + 0x70) + 0x220;
    *(float *)(r + 0xC) = *(float *)&D_L00_00161DEC;
    *(short *)(r + 6) = 0;
}
