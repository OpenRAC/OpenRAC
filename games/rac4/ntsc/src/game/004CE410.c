#include "common.h"

extern s32 D_003D27D8_[];
#define D_003D27D8 (D_003D27D8_[0])

s32 func_004CE410(s32 arg0) {
    return *(s32 *)((arg0 * 4) + (u8 *)&D_003D27D8);
}
