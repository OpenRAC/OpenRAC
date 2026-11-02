#include "include_asm.h"

extern int D_L00_001A904C_2 __attribute__((sda));
extern void func_0033F030(int);

/* Reads the object base from a shared global and, when it is non-zero, calls the handler at a
 * fixed byte offset in it. */
void func_PAL_L00_0030F308(void)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        func_0033F030(value + 0x3BB08);
}

INCLUDE_ASM("asm/pal/levels", func_PAL_L00_0030F338);

extern void func_00345358(int);

/* Reads the object base from a shared global and, when it is non-zero, calls the handler at a
 * fixed byte offset in it. */
void func_PAL_L00_0030F378(void)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        func_00345358(value + 0x3A0B0);
}

