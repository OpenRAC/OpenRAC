#include "include_asm.h"
extern char *D_001A8CFC;
extern void func_0034E478(void *);

void func_PAL_0029CA70(void) {
    if (D_001A8CFC != 0)
        func_0034E478(D_001A8CFC + 0x36F28);
}

extern void func_0034E4F8(void *);

void func_PAL_0029CAA0(void) {
    if (D_001A8CFC != 0)
        func_0034E4F8(D_001A8CFC + 0x36F28);
}

extern void func_00348EC0(void *);

void func_PAL_0029CAD0(void) {
    if (D_001A8CFC != 0)
        func_00348EC0(D_001A8CFC + 0x39160);
}

extern int func_003489E8(void *, int);

int func_PAL_0029CB00(int value) {
    if (D_001A8CFC != 0)
        return func_003489E8(D_001A8CFC + 0x39160, value);
    return 0;
}
