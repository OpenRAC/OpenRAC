#include "include_asm.h"
extern char *D_001A8CFC;
extern int func_00345F70(void *, int, int);

int func_US2_0029D080(int first, int second)
{
    if (D_001A8CFC != 0)
        return func_00345F70(D_001A8CFC + 0x39AF0, first, second);
    return 0;
}

extern int func_00339320(void *, int);

int func_US2_0029D0C8(int value) {
    if (D_001A8CFC != 0)
        return func_00339320(D_001A8CFC + 0x3B418, value);
    return 0;
}
