#include "common.h"

extern s32 D_003D9370_[];
#define D_003D9370 (D_003D9370_[0])

u8 func_004B8908(s32 arg0) {
    return *(s32 *)((arg0 * 0x14) + (u8 *)&D_003D9370);
}
