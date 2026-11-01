#include "include_asm.h"

extern int D_L00_001A904C_1 __attribute__((sda));

void func_PAL_L00_0030E648(int argument)
{
    int value = D_L00_001A904C_1;

    if (value != 0)
        func_0034B000(value + 0x37ED0, argument);
}

