/* NON_MATCHING func_L04_002CF460 -- src/overlays/shared/vendor_002B0068.c
 * Best so far: SIZE ours 140 / retail 148, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
#include "common.h"
extern char *func_0020D348(int);
extern void func_L00_00251E30(void *);

char *func_L04_002CF460(char *src, void *pos)
{
    char scratch[16];
    char *tmp;
    char *moby;
    qcopy(scratch, pos);
    moby = func_0020D348(0x1F3);
    tmp = scratch;
    if (moby != 0) {
        ((unsigned char *)moby)[0x30] = 0xFF;
        moby[0x31] = 1;
        *(unsigned short *)(moby + 0x32) = *(unsigned short *)(src + 0x32);
        *(int *)(moby + 0x40) = 0;
        *(int *)(moby + 0x44) = 0;
        *(float *)(moby + 0x48) = *(float *)(src + 0x48);
        *(long *)(moby + 0x38) = *(long *)(src + 0x38);
        qcopy(moby + 0x10, tmp);
        func_L00_00251E30(moby);
    }
    return moby;
}
