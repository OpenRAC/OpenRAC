#include "common.h"

s32 func_00142230(s32);
s32 func_004F1CF8();

s32 func_004F1DF0(s32 arg0) {
    s32 var_v0;

    var_v0 = 0;
    if (func_004F1CF8() == 0) {
        var_v0 = func_00142230(arg0) != 0;
    }
    return var_v0;
}
