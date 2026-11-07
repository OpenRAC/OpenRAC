#include "common.h"

void func_01EACF38(s32 arg0, u32 arg1) {
    s32 *temp_a0;

    temp_a0 = arg0 + ((arg1 >> 5) * 4);
    *temp_a0 |= 1 << arg1;
}
