#include "include_asm.h"
extern char *D_001A8CFC;
extern void func_00341F38(void *);

void func_US2_0029D958(void) {
    if (D_001A8CFC != 0)
        func_00341F38(D_001A8CFC + 0x3E9C8);
}
