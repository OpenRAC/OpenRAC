#include "include_asm.h"
extern char *D_001A8CFC;
extern int func_00339F28(void *, int);

int func_PAL_0029CC98(int value) {
    if (D_001A8CFC != 0)
        return func_00339F28(D_001A8CFC + 0x3B030, value);
    return 0;
}

extern void func_0033A3E0(void *);

void func_PAL_0029CCD8(void) {
    if (D_001A8CFC != 0)
        func_0033A3E0(D_001A8CFC + 0x3B030);
}

extern void func_0033B910(void *);

void func_PAL_0029CD08(void) {
    if (D_001A8CFC != 0)
        func_0033B910(D_001A8CFC + 0x3B7A8);
}
