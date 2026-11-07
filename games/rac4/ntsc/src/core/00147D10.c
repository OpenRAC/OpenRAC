#include "common.h"

void func_00147D10(f32 *arg0, f32 *arg1, s32 arg2, f32 *arg3) {
    f32 *var_a0;
    f32 *var_a1;
    f32 *var_a3;
    f32 temp_f0;
    f32 temp_f1;
    s32 var_a2;

    var_a0 = arg0;
    var_a1 = arg1;
    var_a2 = arg2;
    var_a3 = arg3;
    if (var_a2 > 0) {
        do {
            temp_f1 = *var_a0;
            var_a2 -= 1;
            temp_f0 = *var_a1;
            var_a0 += 1;
            var_a1 += 1;
            *var_a3 = (temp_f1 * temp_f1) + (temp_f0 * temp_f0);
            var_a3 += 1;
        } while (var_a2 > 0);
    }
}
