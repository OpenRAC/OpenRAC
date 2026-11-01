#include "include_asm.h"
extern char *D_001A8CFC;
extern int func_0033E3A0(void *, int);

int func_US2_0029D758(int value) {
    if (D_001A8CFC != 0)
        return func_0033E3A0(D_001A8CFC + 0x3F3F0, value);
    return 0;
}

extern void func_0033E428(void *);

void func_US2_0029D798(void) {
    if (D_001A8CFC != 0)
        func_0033E428(D_001A8CFC + 0x3F3F0);
}
