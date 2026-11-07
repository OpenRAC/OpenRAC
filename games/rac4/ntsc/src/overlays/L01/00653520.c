/* cflags: -mno-split-addresses */
#include "common.h"

s32 func_L00_00696490(s32, s32);
s32 func_L00_00696588(s32);

s32 func_L01_00653520(s32 arg0) {
    s32 var_v0;

    var_v0 = 0;
    if (func_L00_00696490(arg0, 0) != 0) {
        var_v0 = func_L00_00696588(0);
    }
    return var_v0;
}
