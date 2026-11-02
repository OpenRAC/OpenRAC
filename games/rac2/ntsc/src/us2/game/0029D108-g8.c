#include "include_asm.h"
extern char *D_001A8CFC;
extern void func_00339590(void *);

void func_US2_0029D108(void) {
    if (D_001A8CFC != 0)
        func_00339590(D_001A8CFC + 0x3B418);
}

extern int func_00339F28(void *, int);

int func_US2_0029D138(int value) {
    if (D_001A8CFC != 0)
        return func_00339F28(D_001A8CFC + 0x3AF80, value);
    return 0;
}

extern void func_0033A3E0(void *);

void func_US2_0029D178(void) {
    if (D_001A8CFC != 0)
        func_0033A3E0(D_001A8CFC + 0x3AF80);
}

extern void func_0033B910(void *);

void func_US2_0029D1A8(void) {
    if (D_001A8CFC != 0)
        func_0033B910(D_001A8CFC + 0x3B6F8);
}
