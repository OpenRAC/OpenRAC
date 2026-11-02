#include "include_asm.h"
extern char *D_001A8CFC;
extern int func_003418C0(void *, int);

int func_PAL_0029CEF8(int value) {
    if (D_001A8CFC != 0)
        return func_003418C0(D_001A8CFC + 0x3C210, value);
    return 0;
}

extern void func_003438E8(void *);

void func_PAL_0029CF38(void) {
    if (D_001A8CFC != 0)
        func_003438E8(D_001A8CFC + 0x3C530);
}

extern int func_00343798(void *, int);

int func_PAL_0029CF68(int value) {
    if (D_001A8CFC != 0)
        return func_00343798(D_001A8CFC + 0x3C530, value);
    return 0;
}
