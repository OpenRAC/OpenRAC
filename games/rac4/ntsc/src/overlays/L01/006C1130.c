#include "common.h"

extern s32 D_003C56C0_[];
#define D_003C56C0 (D_003C56C0_[0])

s32 func_L01_006C1130(s32 arg0) {
    return *(s32 *)(((arg0 % 20) * 4) + (u8 *)&D_003C56C0);
}
