#include "include_asm.h"

typedef int TI128 __attribute__((mode(TI)));

extern void func_002B3BA8(char *, int, float, float, int, int);
extern char *func_L20_004448A8(char *, char *);

void func_L20_0039E158(char *arg0) {
    char *s1 = arg0;
    char *s0 = *(char **)(s1 + 0x68);
    char *v;
    int p;

    if (*(int *)(s0 + 0x27C) != 0) {
        *(int *)(s0 + 0x280) = 0;
        func_002B3BA8(arg0, *(int *)(s0 + 0x230), 0.0f, 0.0f, 0, 0);
    } else {
        v = func_L20_004448A8(*(char **)(s1 + 0xB8), s1);
        *(int *)(s0 + 0x280) = (int)v;
        if (v == 0) {
            *(int *)(s0 + 0x280) = *(int *)(s1 + 0xB8);
        }
        p = *(int *)(s0 + 0x280);
        *(int *)(s0 + 0x200) = p;
        *(TI128 *)(s0 + 0x1F0) = *(TI128 *)((char *)p + 0x10);
    }
}
