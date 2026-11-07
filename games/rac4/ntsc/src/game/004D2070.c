#include "common.h"

extern s32 D_003D29B0_[];
#define D_003D29B0 (D_003D29B0_[0])

s32 func_004D2070(s32 arg0) {
    return *(s32 *)((arg0 * 4) + (u8 *)&D_003D29B0);
}
