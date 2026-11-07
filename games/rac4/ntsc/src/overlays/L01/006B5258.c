#include "common.h"

extern s32 D_0038EA70_[];
#define D_0038EA70 (D_0038EA70_[0])

void func_L01_006B5258(s32 arg0, s32 arg1) {
    *(s32 *)((arg0 * 4) + (u8 *)&D_0038EA70) = arg1;
}
