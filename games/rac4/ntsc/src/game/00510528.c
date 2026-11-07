/* cflags: -mno-split-addresses */
#include "common.h"

s32 func_005015B0();
extern s32 D_00220B50_[];
#define D_00220B50 (D_00220B50_[0])

s32 func_00510528(s32 arg0, s32 arg1) {
    s32 var_v0;

    if (arg1 != 3) {
        if (D_00220B50 == arg1) {
            var_v0 = 1;
        } else {
            var_v0 = func_005015B0();
        }
        return var_v0 & 0xFF;
    }
    return 1;
}
