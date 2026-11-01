#include "include_asm.h"

extern int D_L00_001A904C_2 __attribute__((sda));
extern void func_003478F0(int);

/* Reads the object base from a shared global and, when it is non-zero, calls the handler at a
 * fixed byte offset in it. */
void func_PAL_L00_0030F0D0(void)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        func_003478F0(value + 0x39620);
}

extern void func_003465D0(int);

/* Reads the object base from a shared global and, when it is non-zero, calls the handler at a
 * fixed byte offset in it. */
void func_PAL_L00_0030F100(void)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        func_003465D0(value + 0x39BA0);
}

INCLUDE_ASM("asm/pal/levels", func_PAL_L00_0030F130);

INCLUDE_ASM("asm/pal/levels", func_PAL_L00_0030F1B8);

extern void func_00339590(int);

/* Reads the object base from a shared global and, when it is non-zero, calls the handler at a
 * fixed byte offset in it. */
void func_PAL_L00_0030F1F8(void)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        func_00339590(value + 0x3B4C8);
}

INCLUDE_ASM("asm/pal/levels", func_PAL_L00_0030F228);
