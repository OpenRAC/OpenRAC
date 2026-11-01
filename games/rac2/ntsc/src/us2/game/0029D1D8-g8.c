#include "include_asm.h"
extern char *D_001A8CFC;
extern int func_0033B540(void *, int);

int func_US2_0029D1D8(int value) {
    if (D_001A8CFC != 0)
        return func_0033B540(D_001A8CFC + 0x3B6F8, value);
    return 0;
}

extern void func_0033F030(void *);

void func_US2_0029D218(void) {
    if (D_001A8CFC != 0)
        func_0033F030(D_001A8CFC + 0x3BA58);
}
