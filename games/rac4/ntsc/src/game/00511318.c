#include "common.h"

extern s32 D_336838_[];
#define D_336838 (D_336838_[0])

s32 func_00511318(s32 arg0, s32 arg1, s32 arg2) {
    u16 *temp_a3;
    u16 temp_v1;

    temp_a3 = (void *)((arg2 * 2) + (arg0 * 0x600) + (u8 *)&D_336838);
    temp_v1 = *temp_a3;
    *temp_a3 = temp_v1 + 1;
    return (arg0 << 0x18) | (arg1 << 0x1C) | (arg2 & 0xFFF) | (((s32) (temp_v1 << 0x10) >> 4) & 0xFFF000);
}
