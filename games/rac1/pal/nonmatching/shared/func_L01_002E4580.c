/* NON_MATCHING func_L01_002E4580 -- src/overlays/shared/vendor_002B90A8.c
 * Best so far: BYTES 10/160 (93.8% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Scales a moby's velocity (0x10,0x14) by 1 + D_0015EE60*k and subtracts D_0015EE70*29.7 from z (0x18), then cal
 *   Closest (p8, 29/160 bytes): only the order of the 1.0 / 29.7 constant loads and the placement of the addiu $a1
 *   x03 (q30): p15.c is the closest (160 bytes = retail, registers all match, 29 bytes differ): fields written thr
 *   mini9 p19/p20/p22 improve to identical 10/160-byte differences: coefficient uses f0 instead of f1 at 0x14/0x28
 *   Named one local fixes constant scheduling; stopped on three distinct identical wordings. A different FP alloca
 */
#include "common.h"
extern float D_0015EE60 MACRO_ADDR;
extern float D_0015EE70 MACRO_ADDR;
extern int D_L01_001DEE08[];
extern void func_L00_00259B08(int a, int b, int g, int e, float c, float d);

/* damps a moby's velocity by a scale and gravity, then spawns an effect at it */
void func_L01_002E4580(int a, char *moby) {
    int *t = D_L01_001DEE08;
    float one=1.0f;
    float g = D_0015EE70 * 29.7f;
    float s = D_0015EE60;
    s *= -0.050000011920928955f;
    s += one;
    *(float *)(moby + 0x18) = *(float *)(moby + 0x18) - g;
    *(float *)(moby + 0x10) = *(float *)(moby + 0x10) * s;
    *(float *)(moby + 0x14) = *(float *)(moby + 0x14) * s;
    func_L00_00259B08(a, (int)(moby + 0x10), t[0], 0, 0.3f, *(float *)(t + 12));
}
