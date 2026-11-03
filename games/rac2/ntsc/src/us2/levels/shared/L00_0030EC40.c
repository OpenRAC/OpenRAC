#include "include_asm.h"

extern int D_L00_001A904C_2 __attribute__((sda));
extern int func_0033E3A0(int, int);

/* Same bound-global dispatch as its neighbours, with its own base offset and handler. */
int func_US2_L00_0030EC40(int argument)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        return func_0033E3A0(value + 0x3F3F0, argument);
    return 0;
}

void func_US2_L00_0030EC80(void)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        func_0033E428(value + 0x3F3F0);
}

