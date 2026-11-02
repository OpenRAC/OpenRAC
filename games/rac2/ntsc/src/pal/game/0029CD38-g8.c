#include "include_asm.h"
extern char *D_001A8CFC;
extern int func_0033B540(void *, int);

int func_PAL_0029CD38(int value) {
    if (D_001A8CFC != 0)
        return func_0033B540(D_001A8CFC + 0x3B7A8, value);
    return 0;
}

extern void func_0033F030(void *);

void func_PAL_0029CD78(void) {
    if (D_001A8CFC != 0)
        func_0033F030(D_001A8CFC + 0x3BB08);
}
