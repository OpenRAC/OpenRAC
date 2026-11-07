/* cflags: -mno-split-addresses */
#include "common.h"

s32 func_L00_0066B1A8();
s32 func_L00_0066D468(s32, s32, s32);

s32 func_L00_00764920(s32 arg0, s32 arg1) {
    s32 temp_s0;

    temp_s0 = func_L00_0066D468(arg0, arg1, 0);
    func_L00_0066B1A8();
    func_L00_0066B1A8();
    return temp_s0;
}
