#include "common.h"

s32 func_00126A90();

s32 func_01EAA098(s32 *arg0) {
    s32 var_v0;

    var_v0 = 2;
    if (arg0 != NULL) {
        *arg0 = func_00126A90();
        var_v0 = 0;
    }
    return var_v0;
}
