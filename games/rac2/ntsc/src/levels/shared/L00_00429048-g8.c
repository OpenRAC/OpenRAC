#include "common.h"

extern char *D_L00_001B8E10[];
extern char *D_L00_001B1844 __attribute__((sda));
extern float D_L00_001B18E0 MACRO_ADDR;

int func_L00_00429048(int index)
{
    char *y;

    if (*(short *)(D_L00_001B8E10[0] + 0x86) != 0x13)
        return 1;
    y = (char *)(*(int *)(*(int *)(*(char **)(D_L00_001B1844 + index * 32 + 0x1C) + 4) + 0x70) + 0x40);
    if (*(short *)(y + 0x14) == 1 && D_L00_001B18E0 != 0.0f)
        return 1;
    return *(short *)(y + 0x14) == 3;
}
