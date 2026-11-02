/* func_002ED948 in its own unit (flags in config/cflags.txt: -O2 -G0 -ffunction-sections -frac2-qcopy -frac2-qcopy-barrier). */
#include "common.h"


typedef int TI128 __attribute__((mode(TI)));

extern TI128 D_001BFFAFA0;

extern s32 func_00277258(void *, s32, void *, float);

s64 func_002ED948(s32 a0)
{
    s64 r = 0;
    s32 p = a0 + 0x10;

    if (func_00277258(p, 2, a0, 1.5f) != 0) {
        *(TI128 *)p = D_001BFFAFA0;
        r = 1;
    }
    return r;
}
