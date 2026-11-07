#include "common.h"

extern s32 D_308108_[];
#define D_308108 (D_308108_[0])

void func_004A68B8(s32 arg0) {
    *(s32 *)((arg0 * 0xB8) + (u8 *)&D_308108) = 1;
}
