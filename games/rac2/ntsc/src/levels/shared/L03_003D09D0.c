#include "include_asm.h"
#include "common.h"

extern char *func_003113F0(int);
extern void func_002EF818(char *, char *);

char *func_L03_003D09D0(int a0, int a1, void *a2, void *a3, void *a4, int a5, int a6, int a7)
{
    char *obj = func_003113F0(0x9DF);
    char *vars;

    if (obj != 0) {
        *(int *)(obj + 0xB8) = a0;
        *(unsigned char *)(obj + 0x30) = 0xFF;
        vars = *(char **)(obj + 0x68);
        *(short *)(obj + 0x32) = 0xFF;
        func_002EF818(obj + 0xC0, obj + 0xF0);
        qcopy(obj + 0x10, a2);
        qcopy(vars, a3);
        qcopy(vars + 0x10, a4);
        *(int *)(vars + 0x40) = a1;
        *(int *)(vars + 0x28) = a6;
        *(float *)(vars + 0x24) = 1.0f / (float)a5;
        *(int *)(vars + 0x38) = a7;
        *(int *)(vars + 0x34) = 0;
        *(int *)(vars + 0x20) = 0;
    }
    return obj;
}
