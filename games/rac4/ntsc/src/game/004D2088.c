#include "common.h"

extern s32 D_003D29D8_[];
#define D_003D29D8 (D_003D29D8_[0])

s32 func_004D2088(s32 arg0) {
    return *(s32 *)((arg0 * 4) + (u8 *)&D_003D29D8);
}
