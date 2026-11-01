#include "include_asm.h"
extern char *D_001A8CFC;
extern int func_0033D028(void *, int);

int func_US2_0029D7C8(int value) {
    if (D_001A8CFC != 0)
        return func_0033D028(D_001A8CFC + 0x3DA78, value);
    return 0;
}

extern void func_0033D2C0(void *);

void func_US2_0029D808(void) {
    if (D_001A8CFC != 0)
        func_0033D2C0(D_001A8CFC + 0x3DA78);
}
