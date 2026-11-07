/* cflags: -mno-split-addresses */
#include "common.h"

extern s32 D_0021E694_[];
#define D_0021E694 (D_0021E694_[0])

void func_L05_00468718(s32 arg0, s32 arg1) {
    s32 *var_a1;
    s32 temp_a0;
    s32 var_v1;

    temp_a0 = ((D_0021E694 ^ 1) == 0) ? 6 : 0xC;
    var_a1 = arg1 + 0x230;
    if (temp_a0 != 0) {
        var_v1 = temp_a0;
        do {
            if (*var_a1 != 0) {
                *var_a1 = 0;
            }
            var_v1 -= 1;
            var_a1 += 1;
        } while (var_v1 != 0);
    }
}
