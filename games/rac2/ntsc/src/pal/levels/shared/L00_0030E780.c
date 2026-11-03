#include "include_asm.h"

extern int D_L00_001A904C_2 __attribute__((sda));
extern void func_L00_00443DF8(int);

/* Reads a word from a shared global; when it is non-zero, calls a one-argument helper with that word plus a fixed byte offset. */
void func_PAL_L00_0030E780(void)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        func_L00_00443DF8(value + 0x36F28);
}

