#include "include_asm.h"
extern int D_L00_0022F7C0[];
extern int D_001B2590 __attribute__((sda));
extern int D_001B2594 __attribute__((sda));
extern void func_002F8918(int *a, int *b);
extern void func_L00_00373F48(int *a, int *b);
extern void func_L00_00374348(int *a, int *b);
extern void func_L00_00373D40(int *a, int *b);

/* Dispatch on a mode word held in the level's own data, once the module's registry word is set. */
void func_US2_L00_00375A30(void)
{
    int *d = D_L00_0022F7C0;
    int v;

    if (d[0x84 / 4] == 0)
        return;
    v = d[0x50 / 4];
    if (v == 0)
        func_002F8918(&D_001B2590, &D_001B2594);
    else if (v == 1)
        func_L00_00373F48(&D_001B2590, &D_001B2594);
    else if (v == 2)
        func_L00_00374348(&D_001B2590, &D_001B2594);
    else if (v == 3)
        func_L00_00373D40(&D_001B2590, &D_001B2594);
}
