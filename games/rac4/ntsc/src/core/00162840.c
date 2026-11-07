#include "common.h"

extern s32 D_212738_[];
#define D_212738 (D_212738_[0])

void func_00162840(u32 arg0, s32 arg1) {
    if (arg0 < 0xAU) {
        *(s32 *)((arg0 * 4) + (u8 *)&D_212738) = arg1 != 0;
    }
}
