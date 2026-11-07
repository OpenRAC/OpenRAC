#include "common.h"

extern s32 D_308108_[];
#define D_308108 (D_308108_[0])

s32 func_004A7C08(s32 arg0) {
    return *(s32 *)((arg0 * 0xB8) + (u8 *)&D_308108) != 2;
}
