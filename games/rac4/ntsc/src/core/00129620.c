#include "common.h"

extern s32 D_1A6240_[];
#define D_1A6240 (D_1A6240_[0])

s32 func_00129620(s32 arg0) {
    return *(s32 *)((arg0 * 4) + (u8 *)&D_1A6240);
}
