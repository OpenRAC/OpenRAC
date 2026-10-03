/* NON_MATCHING func_L15_002ED318 -- src/overlays/l15_quartu/vendor_0029C1D0.c
 * Best so far: SIZE ours 128 / retail 132, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
#include "common.h"
extern short *D_L15_001AC140[];

void func_L15_002ED318(int index, int enable) {
    short *ids = D_L15_001AC140[index];
    if (ids) {
        do {
            char *moby = D_L15_00160058 + (((unsigned short)*ids & 0x7FFF) << 8);
            if (enable) {
                *(unsigned short *)(moby + 0x34) &= 0xFFFC;
                moby[0x31] = 1;
                *(int *)(moby + 0x94) = *(int *)(*(char **)(moby + 0x24) + 0x10);
            } else {
                *(int *)(moby + 0x94) = 0;
                *(unsigned short *)(moby + 0x34) |= 3;
                moby[0x31] = 0;
            }
        } while (*ids++ >= 0);
    }
}
