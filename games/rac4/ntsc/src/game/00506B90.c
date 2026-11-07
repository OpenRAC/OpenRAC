#include "common.h"

s32 func_00506B90(s32 arg0, s32 arg1) {
    s32 temp_v0;

    temp_v0 = arg1 - 1;
    return (arg0 + temp_v0) & ~temp_v0;
}
