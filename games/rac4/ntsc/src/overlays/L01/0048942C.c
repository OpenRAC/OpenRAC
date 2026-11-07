/* cflags: -mno-split-addresses */
#include "common.h"

extern s32 D_00223C90_[];
#define D_00223C90 (D_00223C90_[0])

void *func_L01_0048942C(s32 arg0, s32 arg1, s32 *arg2) {
    *arg2 = 0x10;
    return &D_00223C90;
}
