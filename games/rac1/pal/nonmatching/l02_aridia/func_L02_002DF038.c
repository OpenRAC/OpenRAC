/* NON_MATCHING func_L02_002DF038 -- src/overlays/l02_aridia/vendor_002A59D8.c
 * Best so far: SIZE ours 140 / retail 136, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
#include "common.h"
extern float D_0015EE6C MACRO_ADDR;
extern float func_001FA748(float, float);
extern void func_L00_002617B0(void *, void *, void *, void *);

void func_L02_002DF038(char *moby)
{
    char zero[16];
    char pos[16];
    char *data = *(char **)(moby + 0x78);
    __builtin_memset(zero, 0, 16);
    qcopy(pos, moby + 0x40);
    *(float *)(moby + 0x48) = func_001FA748(*(float *)(moby + 0x48), D_0015EE6C * 0.08726646f);
    func_L00_002617B0(data + 0x20, zero, pos, moby + 0x40);
}
