#include "common.h"

s32 func_01ED8528();

s32 func_01ED8588(s32 arg0) {
    s32 var_v0;

    var_v0 = 0xFFFF;
    if (arg0 != 0) {
        var_v0 = func_01ED8528();
    }
    return var_v0;
}
