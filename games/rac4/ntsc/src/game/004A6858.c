#include "common.h"

s32 func_004A66E0(s32);
s32 func_00501498(s32);
s32 func_00505AD8(s32);
s32 func_00505B20();
s32 func_00505C90(s32);

void func_004A6858(s32 arg0) {
    s32 temp_s1;

    temp_s1 = func_00505B20();
    func_00505AD8(arg0);
    func_00501498(func_004A66E0(arg0));
    func_00505C90(func_004A66E0(arg0));
    func_00505AD8(temp_s1);
}
