#include "include_asm.h"

extern int D_L00_001A904C_1 __attribute__((sda));
extern int func_0033EA60(int, int);

/* Reads a bound global word; when it is non-zero it calls a helper with that value plus a fixed base offset as the first argument and the function's own argument as the second, otherwise returns 0. */
int func_US2_L00_0030E730(int a0)
{
    int v = D_L00_001A904C_1;

    if (v != 0) {
        return func_0033EA60(v + 0x3BA58, a0);
    }
    return 0;
}

