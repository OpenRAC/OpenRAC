#include "common.h"

s32 func_004C82C8();
s32 func_00501498(s32);
s32 func_00505AD8(s32);
s32 func_00505B20();
s32 func_00505C90(s32);

void func_004C83C0(void) {
    s32 temp_s0;

    temp_s0 = func_00505B20();
    func_00505AD8(0);
    func_00501498(func_004C82C8());
    func_00505C90(func_004C82C8());
    func_00505AD8(temp_s0);
}
