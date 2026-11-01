#include "include_asm.h"

extern int D_L00_001A904C_2 __attribute__((sda));
extern void func_L00_00433F50(int);

/* Reads the object base from a shared global and, when it is non-zero, calls the handler at a
 * fixed byte offset in it. */
void func_US2_L00_0030E930(void)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        func_L00_00433F50(value + 0x3C7E8);
}

