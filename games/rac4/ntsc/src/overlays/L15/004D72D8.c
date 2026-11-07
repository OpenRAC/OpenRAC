#include "common.h"

extern s32 D_0039FE50_[];
#define D_0039FE50 (D_0039FE50_[0])

s32 func_L15_004D72D8(s32 arg0) {
    return *(s32 *)(((arg0 >> 5) * 4) + (u8 *)&D_0039FE50) & (1 << (arg0 & 0x1F));
}
