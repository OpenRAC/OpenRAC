#include "include_asm.h"

extern int D_L00_001A904C_3 __attribute__((sda));
extern int func_00341558(int x);

/* When a global flag is set, derives a pointer from that global plus a fixed offset, runs it through one callee and stores the callee result into a field of the argument. */
int func_PAL_L00_0034ACB8(int *obj)
{
    if (D_L00_001A904C_3 != 0)
        obj[13] = func_00341558(D_L00_001A904C_3 + 0x3C210);
    return 0;
}

