#include "common.h"

extern s32 D_003D2830_[];
#define D_003D2830 (D_003D2830_[0])

s32 func_004CE428(s32 arg0) {
    return *(s32 *)((arg0 * 4) + (u8 *)&D_003D2830);
}
