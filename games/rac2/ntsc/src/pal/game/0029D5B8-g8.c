#include "include_asm.h"
extern char *D_001A8CFC;
extern void func_00340250(void *);

void func_PAL_0029D5B8(void) {
    if (D_001A8CFC != 0)
        func_00340250(D_001A8CFC + 0x3A570);
}

extern void func_00338718(void *, int, int, int);

void func_PAL_0029D5E8(int a0, int a1, int a2) { if (D_001A8CFC != 0) func_00338718(D_001A8CFC + 0x36F28, a0, a1, a2); }

extern void func_003387A0(void *, int, int);

void func_PAL_0029D630(int first, int second) {
    if (D_001A8CFC != 0)
        func_003387A0(D_001A8CFC + 0x36F28, first, second);
}
