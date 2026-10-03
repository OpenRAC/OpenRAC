/* NON_MATCHING func_L08_002DF758 -- src/overlays/l08_batalia/vendor_002B9438.c
 * Best so far: SIZE ours 320 / retail 324, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns moby class 0x1B3, aims it (yaw/pitch via func_L00_001FF860) at the fixed point D_L08_00167640, sets col
 *   p1 (base symbol D_L08_00167500, index 0x50..0x52, which reproduces the -0x140 base register) is size-exact, 28
 *   Unblock: a wording that raises the allocation priority of the constant 1 over the short 0xFF, or qcopy without
 */
#include "common.h"
extern char *func_0020D348(int);
extern float func_001F9D48(void *, void *);
extern float func_L00_001FF860(float, float);
extern void func_L00_00251328(void *, int, int, int);
extern void func_L00_00251E30(void *);
extern float D_L08_00167500[];
extern short D_L08_00161AD0;
extern short D_L08_00161AC8;
extern short D_L08_00161ACC;
extern short D_L08_00161AC4;

/* spawn a moby of class 0x1B3 aimed at the fixed target point */
char *func_L08_002DF758(float a, float b, void *pos, int c, void *vec) {
    char *m = func_0020D348(0x1B3);
    char *d;
    if (m) {
        d = *(char **)(m + 0x78);
        qcopy(m + 0x10, pos);
        *(float *)(m + 0x44) = -func_L00_001FF860(func_001F9D48(D_L08_00167500 + 0x50, m + 0x10), D_L08_00167500[0x52] - *(float *)(m + 0x18));
        *(float *)(m + 0x48) = func_L00_001FF860(D_L08_00167500[0x50] - *(float *)(m + 0x10), D_L08_00167500[0x51] - *(float *)(m + 0x14));
        *(float *)(m + 0x2C) = *(float *)(*(char **)(m + 0x24) + 0x24) * *(float *)&D_L08_00161AD0;
        *(short *)(m + 0x32) = 0xFF;
        m[0x20] = 1;
        m[0x31] = 1;
        *(unsigned char *)(m + 0x30) = 0xFF;
        func_L00_00251328(m, *(int *)&D_L08_00161AC4, *(int *)&D_L08_00161AC8, *(int *)&D_L08_00161ACC);
        *(float *)(d + 0x40) = a;
        *(float *)(d + 0x48) = 70.0f;
        *(float *)(d + 0x44) = b;
        *(int *)(d + 0x4C) = c;
        qcopy(d + 0x20, vec);
        func_L00_00251E30(m);
    }
    return m;
}
