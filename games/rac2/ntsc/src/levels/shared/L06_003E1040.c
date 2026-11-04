#include "include_asm.h"

typedef int s32;

extern s32 D_0029CAE0[];
extern void func_002B0CE0(int, int, int);

void func_L06_003E1040(char *a0)
{
    s32 v = **(s32 **)(a0 + 0x68);

    if (v == 0x18) {
        func_002B0CE0(8, 0x1243, 1);
    } else {
        func_002B0CE0(8, D_0029CAE0[v - 1], 1);
    }
}
