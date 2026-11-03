#include "include_asm.h"

extern char *D_001A8CFC[] __attribute__((sda));
extern void func_00339770(void *, int);
extern char *D_001A8CFC[] __attribute__((sda));
extern void func_00339770(void *, int);

int func_PAL_002CA910(void) {
    if (D_001A8CFC[0] != 0)
        func_00339770(D_001A8CFC[0] + 0x3CF50, 0);
    return 0;
}

int func_PAL_002CA948(void) {
    if (D_001A8CFC[0] != 0)
        func_00339770(D_001A8CFC[0] + 0x3CF50, 1);
    return 0;
}

