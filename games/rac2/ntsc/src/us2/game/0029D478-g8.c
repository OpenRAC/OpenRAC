#include "include_asm.h"
extern char *D_001A8CFC;
extern int func_0033E5C0(void *, int);

int func_US2_0029D478(int value) {
    if (D_001A8CFC != 0)
        return func_0033E5C0(D_001A8CFC + 0x3C7E8, value);
    return 0;
}

extern void func_0033D970(void *);

void func_US2_0029D4B8(void) {
    if (D_001A8CFC != 0)
        func_0033D970(D_001A8CFC + 0x3D1D8);
}

extern int func_0033D868(void *, int);

int func_US2_0029D4E8(int value) {
    if (D_001A8CFC != 0)
        return func_0033D868(D_001A8CFC + 0x3D1D8, value);
    return 0;
}
