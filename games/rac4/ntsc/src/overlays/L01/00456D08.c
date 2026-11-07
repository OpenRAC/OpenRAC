/* cflags: -mno-split-addresses */
#include "common.h"

s32 func_L00_005F6A68();
s32 func_L01_00591BD8(s32, s32, s32);

s32 func_L01_00456D08(s32 arg0, s32 arg1) {
    if (arg1 == 4) {
        func_L01_00591BD8(arg0, 4, -1);
    } else {
        func_L00_005F6A68();
    }
    return 1;
}
