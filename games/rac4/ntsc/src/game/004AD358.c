#include "common.h"

s32 func_004A9E50(s32);
s32 func_004AD2B8();
s32 func_00501498(s32);
s32 func_00505AD8(s32);
s32 func_00505B20();
s32 func_00505C90(s32);
extern s32 D_308B60_[];
#define D_308B60 (D_308B60_[0])

void func_004AD358(s32 arg0) {
    s32 temp_s0;

    if (D_308B60 == 0) {
        func_004AD2B8();
    }
    D_308B60 |= 1 << arg0;
    temp_s0 = func_00505B20();
    func_00505AD8(arg0);
    func_00501498(func_004A9E50(arg0));
    func_00505C90(func_004A9E50(arg0));
    func_00505AD8(temp_s0);
}
