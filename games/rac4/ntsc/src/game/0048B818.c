#include "common.h"

u32 func_0048B818(s32 arg0, u32 *arg1, u32 arg2) {
    u32 temp_a0;
    u32 temp_v0;

    temp_v0 = *arg1;
    temp_a0 = temp_v0 + ((arg0 + 0xF) & ~0xF);
    *arg1 = temp_a0;
    return (temp_a0 >= arg2) ? 0U : temp_v0;
}
