#include "include_asm.h"

extern int D_L00_001A904C_2 __attribute__((sda));

void func_US2_L00_0030F228(void)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        func_003399B0(value + 0x3CEA0);
}

extern int func_003398A8(int, int);

/* Same bound-global dispatch as its neighbours, with its own base offset and handler. */
int func_US2_L00_0030F258(int argument)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        return func_003398A8(value + 0x3CEA0, argument);
    return 0;
}

