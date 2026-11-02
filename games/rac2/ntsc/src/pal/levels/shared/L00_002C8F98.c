#include "include_asm.h"

extern char D_00189E20_4[];
extern void func_L00_002C42E0(int a0, int a1);
extern void func_L00_002AF7D0(void);

/* Gates the level transition on the shared block: the state word must be 0, 2 or 3, the byte at
 * +0x22B8 non-zero, the counter at +0x1D8 below 6 and the timer at +0x24B8 below 0.7. */
int func_PAL_L00_002C8F98(void)
{
    char *p = D_00189E20_4;
    int state = *(int *)(p + 0x2294);

    if (state != 2 && state != 3 && state != 0)
        return 0;
    if (*(unsigned char *)(p + 0x22B8) == 0)
        return 0;
    if (*(int *)(p + 0x1D8) >= 5)
        return 0;
    if (*(float *)(p + 0x24B8) < 0.7f) {
        if (*(unsigned char *)(p + 0x22BB) != 0)
            return 0;
        func_L00_002AF7D0();
        *(int *)(p + 0xEA4) = 2;
        func_L00_002C42E0(0x22, 1);
        return 1;
    }
    return 0;
}

