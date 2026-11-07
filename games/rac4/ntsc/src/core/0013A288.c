#include "common.h"

s32 func_0013A160(s32, s32, s32, s32);
extern s32 D_00167BF8_[];
#define D_00167BF8 (D_00167BF8_[0])

s32 func_0013A288(s32 arg0, s32 arg1, s32 arg2) {
    s32 temp_v0;

    temp_v0 = func_0013A160(arg0, arg1, arg2, 0x40);
    if (temp_v0 == 0) {
        D_00167BF8 = 0xB;
    }
    return temp_v0;
}
