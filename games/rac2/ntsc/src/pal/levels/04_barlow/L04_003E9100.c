#include "include_asm.h"

extern void func_002A7470(void *, int, int, int);

void func_PAL_L04_003E9100(char *a0)
{
    char *s0 = *(char **)(a0 + 0x68);

    if (*(char **)(s0 + 0x2AC) != 0) {
        func_002A7470(*(char **)(s0 + 0x2AC), 9, 0, 2);
        *(int *)(*(char **)(*(char **)(s0 + 0x2AC) + 0x68) + 0x27C) = 0;
        *(char **)(s0 + 0x2AC) = 0;
    }
}

