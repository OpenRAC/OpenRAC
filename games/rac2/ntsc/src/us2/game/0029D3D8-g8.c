#include "include_asm.h"
extern char *D_001A8CFC;
extern void func_003438E8(void *);

void func_US2_0029D3D8(void) {
    if (D_001A8CFC != 0)
        func_003438E8(D_001A8CFC + 0x3C480);
}

extern int func_00343798(void *, int);

int func_US2_0029D408(int value) {
    if (D_001A8CFC != 0)
        return func_00343798(D_001A8CFC + 0x3C480, value);
    return 0;
}
