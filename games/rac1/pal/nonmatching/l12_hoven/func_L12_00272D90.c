/* NON_MATCHING func_L12_00272D90 -- src/overlays/l12_hoven/mobyutil_00272D90.c
 * Best so far: BYTES 17/148 (88.5% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
#include "common.h"

extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_L00_00259B88(void *, void *, void *, void *, float);

void func_L12_00272D90(char *moby, void *target) {
    float position[4];
    char result[16];
    qcopy(position, moby + 0x10);
    position[0] += func_001F9F90(*(float *)(moby + 0x48)) * 100.0f;
    position[1] += func_001F9FA8(*(float *)(moby + 0x48)) * 100.0f;
    func_L00_00259B88(moby, target, position, result, 1.0f);
}
