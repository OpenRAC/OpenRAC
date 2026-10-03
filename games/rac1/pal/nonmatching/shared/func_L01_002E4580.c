/* NON_MATCHING func_L01_002E4580 -- src/overlays/shared/vendor_002B90A8.c
 * Best so far: SIZE ours 164 / retail 160, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Scales a moby's velocity (0x10,0x14) by 1 + D_0015EE60*k and subtracts D_0015EE70*29.7 from z (0x18), then cal
 *   Closest (p8, 29/160 bytes): only the order of the 1.0 / 29.7 constant loads and the placement of the addiu $a1
 */
#include "common.h"
extern float D_0015EE60 MACRO_ADDR;
extern float D_0015EE70 MACRO_ADDR;
extern int D_L01_001DEE08[];
extern void func_L00_00259B08(int a, int b, int g, int e, float c, float d);

/* damps a moby's velocity by a scale and gravity, then spawns an effect at it */
void func_L01_002E4580(int a, char *moby) {
    float *v = (float *)(moby + 0x10);
    float s = D_0015EE60 * -0.050000011920928955f + 1.0f;
    float g = D_0015EE70 * 29.7f;
    v[2] = v[2] - g;
    v[0] = v[0] * s;
    v[1] = v[1] * s;
    func_L00_00259B08(a, (int)v, D_L01_001DEE08[0], 0, 0.3f, *(float *)((char *)D_L01_001DEE08 + 0x30));
}
