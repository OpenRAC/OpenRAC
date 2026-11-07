/* cflags: -mno-split-addresses */
#include "common.h"

s32 func_01F0C090(void *, s32);
extern s32 D_0019E698_[];
#define D_0019E698 (D_0019E698_[0])

void func_01F0AD14(void) {
    func_01F0C090(&D_0019E698, 0x70);
}
