#include "common.h"

extern s32 D_003D9528_[];
#define D_003D9528 (D_003D9528_[0])

u8 func_004BA600(s32 arg0) {
    return *(s32 *)((arg0 * 0x14) + (u8 *)&D_003D9528);
}
