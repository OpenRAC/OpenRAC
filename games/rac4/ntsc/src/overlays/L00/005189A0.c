/* cflags: -mno-split-addresses */
#include "common.h"

s32 func_L00_005189A0(s32 arg0, s32 arg1) {
    s32 var_v1;

    var_v1 = 0;
    if ((arg1 != 1) && (arg1 != 5) && (arg1 != 6) && (arg1 != 8)) {
        var_v1 = ((arg1 ^ 7) == 0) ? 0 : 0x2722;
    }
    return var_v1;
}
