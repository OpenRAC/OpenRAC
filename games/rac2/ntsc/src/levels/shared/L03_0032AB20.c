#include "common.h"

extern unsigned char D_L03_00189E20[];
extern int D_L03_001A8FB4 MACRO_ADDR;

int func_L03_0032AB20(int arg)
{
    unsigned char *p;

    if (D_L03_001A8FB4 != arg)
        return 0;
    p = D_L03_00189E20;
    if (*(unsigned char *)(p + 0x123A) != 0)
        return 0;
    if (*(int *)(p + 0x24CC) == 0 || *(int *)(p + 0x229C) == 0x14)
        return 0;
    return 1;
}
