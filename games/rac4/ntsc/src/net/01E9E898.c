/* cflags: -mno-split-addresses */
#include "common.h"

s32 func_0011A518(s32, s32, s32);

s32 func_01E9E898(s32 arg0) {
    s32 var_v0;

    var_v0 = 0x17;
    if (arg0 != 0) {
        func_0011A518(arg0, 0, 0x10);
        var_v0 = 0;
    }
    return var_v0;
}
