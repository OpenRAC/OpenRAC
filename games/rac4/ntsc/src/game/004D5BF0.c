#include "common.h"

s32 func_00462080(s32);
extern s32 D_1DFCA0_[];
#define D_1DFCA0 (D_1DFCA0_[0])

void func_004D5BF0(s32 arg0, s32 arg1) {
    if (D_1DFCA0 != arg1) {
        func_00462080(arg1);
    }
}
