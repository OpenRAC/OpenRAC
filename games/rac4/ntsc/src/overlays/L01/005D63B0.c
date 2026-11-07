#include "common.h"

f32 func_L01_005D6400(s32);
extern s32 D_00297680_[];
#define D_00297680 (D_00297680_[0])

f32 func_L01_005D63B0(s32 arg0) {
    s32 temp_a0;

    if (arg0 != -1) {
        temp_a0 = *(s32 *)((arg0 * 4) + (u8 *)&D_00297680);
        if (temp_a0 != 0) {
            return func_L01_005D6400(temp_a0);
        }
    }
    return -1.0f;
}
