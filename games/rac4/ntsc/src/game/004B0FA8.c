#include "common.h"

s32 func_004B0F08();
s32 func_00501498(s32);
s32 func_00505AD8(s32);
s32 func_00505B20();
s32 func_00505C90(s32);
extern s32 D_309650_[];
#define D_309650 (D_309650_[0])

void func_004B0FA8(s32 arg0) {
    s32 temp_s0;

    if (D_309650 == 0) {
        temp_s0 = func_00505B20();
        func_00505AD8(4);
        func_00501498(func_004B0F08());
        func_00505C90(func_004B0F08());
        func_00505AD8(temp_s0);
    }
    D_309650 |= 1 << arg0;
}
