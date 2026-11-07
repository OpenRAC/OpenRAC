#include "common.h"

void func_00500CF0(f32 *arg0, f32 *arg1, f32 *arg2, f32 fparg0) {
    f32 temp_f0;

    temp_f0 = *arg1;
    *arg0 = temp_f0 + (fparg0 * (*arg2 - temp_f0));
}
