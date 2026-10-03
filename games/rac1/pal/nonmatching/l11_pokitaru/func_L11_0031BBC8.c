/* NON_MATCHING func_L11_0031BBC8 -- src/overlays/l11_pokitaru/vendor_00312BD8.c
 * Best so far: SIZE ours 120 / retail 124, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
#include "common.h"
extern short *D_L11_001AC540[];
extern char *D_L11_00160058;

short func_L11_0031BBC8(int index) {
    char *base = D_L11_00160058;
    short *ids = D_L11_001AC540[index];
    short count = 0;
    for (;;) {
        int id = *(unsigned short *)ids;
        int index = id;
        char *moby;
        index &= 0x7FFF;
        moby = base + (index << 8);
        if (*(short *)(moby + 0xA6) == 0x527 && (unsigned char)moby[0x20] == 5) {
            count++;
        }
        if ((short)id < 0) break;
        ids++;
    }
    return count;
}
