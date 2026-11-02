#include "include_asm.h"

extern int D_L00_001A904C_2 __attribute__((sda));

void func_PAL_L00_0030FE30(void)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        func_003399B0(value + 0x3CF50);
}

extern int func_003398A8(int, int);

/* Same bound-global dispatch as its neighbours, with its own base offset and handler. */
int func_PAL_L00_0030FE60(int argument)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        return func_003398A8(value + 0x3CF50, argument);
    return 0;
}

