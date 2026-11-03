#include "include_asm.h"
#include "common.h"

extern int D_L09_001B35E0[] MACRO_ADDR;
extern int D_L09_001B36E0[] MACRO_ADDR;
extern int D_L09_001B3720[] MACRO_ADDR;
extern int D_L09_00255E90[];
extern void func_00291450(int);

void func_L09_003FB860(int index)
{
    int h;

    D_L09_001B35E0[index] = 0;
    D_L09_00255E90[index * 2] = 0;
    h = D_L09_001B36E0[index];
    *(int *)((u8 *)D_L09_00255E90 + index * 8 + 4) = 0;
    if (h != -1) {
        func_00291450(h);
        D_L09_001B36E0[index] = -1;
    }
    if (D_L09_001B3720[index] != -1) {
        func_00291450(D_L09_001B3720[index]);
        D_L09_001B3720[index] = -1;
    }
}
