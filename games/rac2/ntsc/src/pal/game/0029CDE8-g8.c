#include "include_asm.h"
extern char *D_001A8CFC;
extern void func_00345358(void *);

void func_PAL_0029CDE8(void) {
    if (D_001A8CFC != 0)
        func_00345358(D_001A8CFC + 0x3A0B0);
}

extern int func_00344478(void *, int);

int func_PAL_0029CE18(int value) {
    if (D_001A8CFC != 0)
        return func_00344478(D_001A8CFC + 0x3A0B0, value);
    return 0;
}
