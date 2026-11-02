#include "include_asm.h"
extern char *D_001A8CFC;
extern void func_00341C20(void *);

void func_US2_0029D2F8(void) {
    if (D_001A8CFC != 0)
        func_00341C20(D_001A8CFC + 0x3C160);
}

extern int func_003418C0(void *, int);

int func_US2_0029D328(int value) {
    if (D_001A8CFC != 0)
        return func_003418C0(D_001A8CFC + 0x3C160, value);
    return 0;
}

void func_US2_0029D368(void) {
    if (D_001A8CFC != 0)
        func_00341C20(D_001A8CFC + 0x3C160);
}

int func_US2_0029D398(int value) {
    if (D_001A8CFC != 0)
        return func_003418C0(D_001A8CFC + 0x3C160, value);
    return 0;
}
