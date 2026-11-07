#include "common.h"

extern s32 D_3091A0_[];
#define D_3091A0 (D_3091A0_[0])

void func_004AD868(s32 arg0) {
    *(s32 *)((arg0 * 0x50) + (u8 *)&D_3091A0) = 2;
}
