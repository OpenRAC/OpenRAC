#include "include_asm.h"

extern int D_L00_001A904C_2 __attribute__((sda));
extern void func_00339590(int);

/* Reads the object base from a shared global and, when it is non-zero, calls the handler at a
 * fixed byte offset in it. */
void func_US2_L00_0030E5F0(void)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        func_00339590(value + 0x3B418);
}

extern int D_L00_001A904C_1 __attribute__((sda));
extern int func_00339F28(int, int);

/* Reads a bound global word; when it is non-zero it calls a helper with that value plus a fixed base offset as the first argument and the function's own argument as the second, otherwise returns 0. */
int func_US2_L00_0030E620(int a0)
{
    int v = D_L00_001A904C_1;

    if (v != 0) {
        return func_00339F28(v + 0x3AF80, a0);
    }
    return 0;
}

extern void func_0033A3E0(int);

/* Reads the object base from a shared global and, when it is non-zero, calls the handler at a
 * fixed byte offset in it. */
void func_US2_L00_0030E660(void)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        func_0033A3E0(value + 0x3AF80);
}

extern void func_0033B910(int);

/* Reads the object base from a shared global and, when it is non-zero, calls the handler at a
 * fixed byte offset in it. */
void func_US2_L00_0030E690(void)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        func_0033B910(value + 0x3B6F8);
}

