/* cflags: -mno-split-addresses */
#include "common.h"

s32 func_004F1308(s32);
extern s32 D_221ED4_[];
#define D_221ED4 (D_221ED4_[0])

s32 func_004F20C8(void) {
    func_004F1308(D_221ED4 + 0x48);
    return 1;
}
