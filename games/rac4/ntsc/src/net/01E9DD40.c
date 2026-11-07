/* cflags: -mno-split-addresses */
#include "common.h"

s32 func_01EA9618(s32, s32, s32);

s32 func_01E9DD40(s32 arg0, s32 arg1, s32 arg2) {
    return (func_01EA9618(arg0, arg1, arg2 & 0xFFFF) == 0) ? 0 : -1;
}
