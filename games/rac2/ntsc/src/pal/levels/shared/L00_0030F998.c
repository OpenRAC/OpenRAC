#include "include_asm.h"

INCLUDE_ASM("asm/pal/levels", func_PAL_L00_0030F998);

extern int D_L00_001A904C_2 __attribute__((sda));

void func_PAL_L00_0030F9D8(void)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        func_00342758(value + 0x3E810);
}

