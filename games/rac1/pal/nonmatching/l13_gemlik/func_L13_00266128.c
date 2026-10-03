/* NON_MATCHING func_L13_00266128 -- src/overlays/l13_gemlik/mobyutil_00266128.c
 * Best so far: SIZE ours 80 / retail 84, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
#include "common.h"

extern char *D_L13_00160058;
extern char *D_L13_00160060;

void func_L13_00266128(int id, int state) {
    char *moby = D_L13_00160058;
    if ((unsigned long)D_L13_00160060 >= (unsigned long)moby) {
        do {
            if (*(short *)(moby + 0xA6) == id) {
                if (moby[0x20] >= 0) moby[0x20] = state;
            }
            moby += 0x100;
        } while ((unsigned long)D_L13_00160060 >= (unsigned long)moby);
    }
}
