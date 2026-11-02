#include "include_asm.h"
extern char *D_001A8CFC;
extern int func_003418C0(void *, int);

int func_PAL_0029CE88(int value) {
    if (D_001A8CFC != 0)
        return func_003418C0(D_001A8CFC + 0x3C210, value);
    return 0;
}
