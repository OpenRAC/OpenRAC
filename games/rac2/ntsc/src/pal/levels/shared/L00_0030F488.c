#include "include_asm.h"

INCLUDE_ASM("asm/pal/levels", func_PAL_L00_0030F488);

extern int D_L00_001A904C_2 __attribute__((sda));
extern void func_003438E8(int);

/* Reads the object base from a shared global and, when it is non-zero, calls the handler at a
 * fixed byte offset in it. */
void func_PAL_L00_0030F4C8(void)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        func_003438E8(value + 0x3C530);
}

INCLUDE_ASM("asm/pal/levels", func_PAL_L00_0030F4F8);
