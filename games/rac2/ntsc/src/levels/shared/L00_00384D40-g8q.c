#include "include_asm.h"
typedef int Q128 __attribute__((mode(TI)));

extern char *func_0029EFD0(int);
extern void func_002AEC18(void *a0, int a1);
extern void func_002A7750(char *, int, int);
extern void func_L00_00383B90(void *);
extern int D_L00_001AEE0C __attribute__((sda));
extern float D_L00_001AEE08 __attribute__((sda));

char *func_L00_00384D40(char *owner, void *pos, void *vel)
{
    char *o = func_0029EFD0(0x5D8);

    if (o != 0) {
        char *st = *(char **)(o + 0x68);

        func_002AEC18(o, D_L00_001AEE0C);
        *(unsigned char *)(o + 0x30) = 0xFF;
        *(char **)(o + 0xB8) = owner;
        *(unsigned short *)(o + 0x32) = 0xFF;
        *(float *)(o + 0x2C) = *(float *)(*(char **)(o + 0x24) + 0x24) * D_L00_001AEE08;
        *(void (**)(void *))(o + 0x64) = func_L00_00383B90;
        *(Q128 *)(o + 0x10) = *(Q128 *)pos;
        *(Q128 *)st = *(Q128 *)vel;
        func_002A7750(o, 3, -1);
    }
    return o;
}
