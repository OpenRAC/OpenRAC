#include "common.h"
extern char *D_L02_001B1190[] SDATA(D_L02_001B1190);

int func_L02_003FCBF0(char *a0) {
    char *obj = *(char **)(a0 + 0x68);
    char *sub = D_L02_001B1190[*(short *)(obj + 0x120)];
    char *e = *(char **)(sub + 0x4) + *(short *)(obj + 0x122) * 0x14;
    int ok;

    ok = *(int *)(obj + 0x90) == *(short *)(e + 0);
    if (*(int *)(obj + 0x94) != *(short *)(e + 2))
        ok = 0;
    if (*(int *)(obj + 0x98) != *(short *)(e + 4))
        ok = 0;
    ok &= 1;
    if (*(int *)(obj + 0x9C) != *(short *)(e + 6))
        ok = 0;
    if (*(unsigned char *)(e + 0x10))
        ok &= *(int *)(obj + 0xA0);
    if (*(unsigned char *)(e + 0x11))
        ok &= *(int *)(obj + 0xA4);
    return *(int *)(obj + 0x60) == 0 && ok != 0;
}
