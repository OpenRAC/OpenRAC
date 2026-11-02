#include "include_asm.h"
#include "common.h"
extern s32 func_003384C0();
extern s32 D_001A8CFC;

void func_PAL_0029BFF0(s32 arg0, s32 arg1) {
    s32 a;
    s32 b;

    a = arg0;
    b = arg1;
    if (D_001A8CFC != 0) {
        func_003384C0(D_001A8CFC + 0x36F28, a, b);
    }
}
