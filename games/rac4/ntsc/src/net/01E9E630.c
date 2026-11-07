#include "common.h"

s32 func_01EA0BD0();

s32 func_01E9E630(s32 *arg0) {
    s32 var_v0;

    var_v0 = 0x17;
    if (arg0 != NULL) {
        *arg0 = func_01EA0BD0();
        var_v0 = 0;
    }
    return var_v0;
}
