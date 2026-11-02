#include "include_asm.h"

extern int D_L00_001A904C_2 __attribute__((sda));
extern int func_0033D028(int, int);

/* Same bound-global dispatch as its neighbours, with its own base offset and handler. */
int func_US2_L00_0030ECB0(int argument)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        return func_0033D028(value + 0x3DA78, argument);
    return 0;
}

void func_US2_L00_0030ECF0(void)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        func_0033D2C0(value + 0x3DA78);
}

