/* cflags: -mno-split-addresses */
#include "common.h"

s32 func_L01_0050AF48(s32, s32, s32, s32);
s32 func_L01_0050B658(s32, s32, s32);

void func_L01_0050B608(s32 arg0, s32 arg1) {
    if (func_L01_0050B658(arg0, arg1, 0) == 0) {
        func_L01_0050AF48(arg0, arg1, -1, 0);
    }
}
