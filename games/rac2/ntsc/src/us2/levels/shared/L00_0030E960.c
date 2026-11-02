#include "include_asm.h"

extern int D_L00_001A904C_1 __attribute__((sda));
extern int func_0033E5C0(int, int);

/* Reads a bound global word; when it is non-zero it calls a helper with that value plus a fixed base offset as the first argument and the function's own argument as the second, otherwise returns 0. */
int func_US2_L00_0030E960(int a0)
{
    int v = D_L00_001A904C_1;

    if (v != 0) {
        return func_0033E5C0(v + 0x3C7E8, a0);
    }
    return 0;
}

extern int D_L00_001A904C_2 __attribute__((sda));
extern void func_0033D970(int);

/* Reads the object base from a shared global and, when it is non-zero, calls the handler at a
 * fixed byte offset in it. */
void func_US2_L00_0030E9A0(void)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        func_0033D970(value + 0x3D1D8);
}

extern int func_L00_00433198(int, int);

/* Reads a bound global word; when it is non-zero it calls a helper with that value plus a fixed base offset as the first argument and the function's own argument as the second, otherwise returns 0. */
int func_US2_L00_0030E9D0(int a0)
{
    int v = D_L00_001A904C_1;

    if (v != 0) {
        return func_L00_00433198(v + 0x3D1D8, a0);
    }
    return 0;
}

