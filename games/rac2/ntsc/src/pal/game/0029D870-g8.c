#include "include_asm.h"
extern char *D_001A8CFC;
extern void func_003399B0(void *);

void func_PAL_0029D870(void) {
    if (D_001A8CFC != 0)
        func_003399B0(D_001A8CFC + 0x3CF50);
}

extern int func_003398A8(void *, int);

int func_PAL_0029D8A0(int value) {
    if (D_001A8CFC != 0)
        return func_003398A8(D_001A8CFC + 0x3CF50, value);
    return 0;
}
