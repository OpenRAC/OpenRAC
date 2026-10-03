/* NON_MATCHING func_L11_00311260 -- src/overlays/l11_pokitaru/vendor_002CC828.c
 * Best so far: SIZE ours 176 / retail 180, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
#include "common.h"

extern char *func_0020D348(int);
extern void func_00213DE0(void *, int, int, int);
extern void func_L00_00251E30(void *);
extern void func_L00_0025E210(void *);

char *func_L11_00311260(char *owner) {
    char *moby = func_0020D348(0x4C3);
    if (moby != 0) {
        char *data;
        *(short *)(moby + 0x32) = 0xFF;
        data = *(char **)(moby + 0x78);
        ((unsigned char *)moby)[0x30] = 0xFF;
        moby[0x31] = 1;
        moby[0x20] = 0;
        moby[0xBC] = 0;
        qcopy(moby + 0x10, owner + 0x10);
        qcopy(moby + 0x40, owner + 0x40);
        *(char **)(data + 0x70) = owner;
        if ((unsigned char)moby[0x53] != 2) {
            func_00213DE0(moby, 2, 0xD, 0);
        }
        func_L00_00251E30(moby);
        func_L00_0025E210(moby);
    }
    return moby;
}
