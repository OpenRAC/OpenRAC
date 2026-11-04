#include "include_asm.h"

extern int D_L00_001A904C_2 __attribute__((sda));

void func_US2_L00_0030EE40(void)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        func_00341F38(value + 0x3E9C8);
}

