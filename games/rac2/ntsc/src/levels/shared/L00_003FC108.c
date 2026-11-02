#include "include_asm.h"
typedef int TI128 __attribute__((mode(TI)));

extern char * func_0029EFD0(int);
extern void func_00282F20(void *, void *, float);
extern void func_00282CC0(void *, void *, void *);
extern float func_00283CE0(int);
extern void func_L00_003FC820(void *);

void *func_L00_003FC108(char *a0, char *a1, TI128 *a2, char *a3)
{
    char *o = func_0029EFD0(0xD5E);

    if (o != 0) {
        char *s0 = *(char **)(o + 0x68);
        float tmp[4];

        *(unsigned char *)(o + 0x30) = 0xFF;
        *(unsigned short *)(o + 0x32) = 0xFF;
        *(unsigned char *)(o + 0x31) = 1;
        *(unsigned char *)(o + 0x20) = 0;
        *(unsigned short *)(o + 0x34) |= 0x41;
        func_00282F20(s0 + 0x20, a1, 0.13333334f);
        *(TI128 *)(o + 0x10) = *a2;
        func_00282F20(tmp, s0 + 0x20, -1.5f);
        func_00282CC0(s0 + 0x50, a0, tmp);
        *(int *)(s0 + 0x38) = 0x14;
        *(float *)(s0 + 0x3C) = 1.0f / func_00283CE0(0x14);
        *(char **)(s0 + 0x34) = a3;
        func_L00_003FC820(o + 0x10);
    }
    return o;
}
