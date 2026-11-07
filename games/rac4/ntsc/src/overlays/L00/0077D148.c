/* cflags: -mno-split-addresses */
#include "common.h"

s32 func_L00_0077CFE0(s32, s32);

s32 func_L00_0077D148(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    s32 temp_s0;

    temp_s0 = func_L00_0077CFE0(func_L00_0077CFE0(arg2, 4) + 4, arg3);
    return (func_L00_0077CFE0(arg0 + 0x20, arg3) + (temp_s0 * arg1)) - arg0;
}
