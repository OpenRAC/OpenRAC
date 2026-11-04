#include "include_asm.h"
extern char *D_001A8CFC;
extern void func_003399B0(void *);

void func_US2_0029DD10(void) {
    if (D_001A8CFC != 0)
        func_003399B0(D_001A8CFC + 0x3CEA0);
}

extern int func_003398A8(void *, int);

int func_US2_0029DD40(int value) {
    if (D_001A8CFC != 0)
        return func_003398A8(D_001A8CFC + 0x3CEA0, value);
    return 0;
}
