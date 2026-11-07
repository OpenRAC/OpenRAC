#include "common.h"

extern s32 D_003D2988_[];
#define D_003D2988 (D_003D2988_[0])

s32 func_004D2058(s32 arg0) {
    return *(s32 *)((arg0 * 4) + (u8 *)&D_003D2988);
}
