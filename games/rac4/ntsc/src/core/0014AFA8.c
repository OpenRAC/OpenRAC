#include "common.h"

f32 func_0014AFA8(f32 *arg0, f32 fparg0, f32 fparg1, f32 fparg2) {
    f32 temp_f2;
    f32 temp_f3;
    f32 var_f14;

    temp_f2 = ((fparg0 + fparg2) * 0.5f) - fparg1;
    temp_f3 = (fparg2 - fparg0) * 0.5f;
    if (temp_f2 != 0.0f) {
        var_f14 = -temp_f3 / (2.0f * temp_f2);
    } else {
        var_f14 = 0.0f;
    }
    *arg0 = (((temp_f2 * var_f14) + temp_f3) * var_f14) + fparg1;
    return var_f14;
}
