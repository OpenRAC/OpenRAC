/* cflags: -mno-split-addresses */
#include "common.h"

s32 func_L00_005DD4A0(s32);

void func_L04_00498038(s32 arg0, s32 arg1) {
    s32 *var_s0;
    s32 temp_v0;
    s32 var_s1;

    var_s0 = arg1 + 0x2F0;
    var_s1 = 1;
    do {
        temp_v0 = *var_s0;
        var_s1 -= 1;
        if (temp_v0 != 0) {
            func_L00_005DD4A0(temp_v0);
            *var_s0 = 0;
        }
        var_s0 += 1;
    } while (var_s1 >= 0);
}
