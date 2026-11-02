#include "include_asm.h"

extern char *D_001A8CFC[] __attribute__((sda));
extern void func_00339770(void *, int);
extern char *D_001A8CFC[] __attribute__((sda));
extern void func_00339770(void *, int);

int func_US2_002CAA38(void) {
    if (D_001A8CFC[0] != 0)
        func_00339770(D_001A8CFC[0] + 0x3CEA0, 0);
    return 0;
}

int func_US2_002CAA70(void) {
    if (D_001A8CFC[0] != 0)
        func_00339770(D_001A8CFC[0] + 0x3CEA0, 1);
    return 0;
}

INCLUDE_ASM("asm/us2/game", func_US2_002CAAA8);
