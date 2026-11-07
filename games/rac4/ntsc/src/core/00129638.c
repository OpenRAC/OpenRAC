#include "common.h"

extern s32 D_1A6240_[];
#define D_1A6240 (D_1A6240_[0])

s32 func_00129638(s32 arg0, s32 arg1) {
    *(s32 *)((arg0 * 4) + (u8 *)&D_1A6240) = arg1;
    return arg1;
}
