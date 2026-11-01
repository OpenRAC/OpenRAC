#include "include_asm.h"
extern char *D_001A8CFC;
extern void func_00341C20(void *);

void func_PAL_0029CEC8(void) {
    if (D_001A8CFC != 0)
        func_00341C20(D_001A8CFC + 0x3C210);
}
