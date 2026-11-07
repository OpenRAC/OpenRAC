#include "common.h"

extern s32 D_3098F0_[];
#define D_3098F0 (D_3098F0_[0])

void func_004B6950(s32 arg0) {
    *(s32 *)((arg0 * 0x1F8) + (u8 *)&D_3098F0) = 0;
}
