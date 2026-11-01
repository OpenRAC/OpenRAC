#include "include_asm.h"

extern int D_L00_001A904C_2 __attribute__((sda));
extern int func_L00_00433560(int, int);

/* Same bound-global dispatch as its neighbours, with its own base offset and handler. */
int func_US2_L00_0030ED20(int argument)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        return func_L00_00433560(value + 0x3EB50, argument);
    return 0;
}

void func_US2_L00_0030ED60(void)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        func_0033DCC8(value + 0x3EB50);
}

extern int func_003424C0(int, int);

/* Same bound-global dispatch as its neighbours, with its own base offset and handler. */
int func_US2_L00_0030ED90(int argument)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        return func_003424C0(value + 0x3E760, argument);
    return 0;
}

void func_US2_L00_0030EDD0(void)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        func_00342758(value + 0x3E760);
}

extern int func_00341EE8(int, int);

/* Same bound-global dispatch as its neighbours, with its own base offset and handler. */
int func_US2_L00_0030EE00(int argument)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        return func_00341EE8(value + 0x3E9C8, argument);
    return 0;
}

