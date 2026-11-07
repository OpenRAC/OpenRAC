#include "common.h"

s32 func_01EBDE60(s32);

s32 func_01EB7770(s32 *arg0) {
    s32 var_v0;

    var_v0 = 2;
    if (arg0 != NULL) {
        var_v0 = func_01EBDE60(*arg0);
    }
    return var_v0;
}
