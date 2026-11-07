#include "common.h"

extern s32 D_3367B8_[];
#define D_3367B8 (D_3367B8_[0])

s32 func_00510C18(s32 arg0) {
    return *(s32 *)((arg0 * 4) + (u8 *)&D_3367B8);
}
