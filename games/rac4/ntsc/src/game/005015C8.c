#include "common.h"

s32 func_005015C8(s32 *arg0) {
    s32 temp_v0;

    temp_v0 = *arg0;
    if (temp_v0 != 0) {
        *arg0 = temp_v0 - 1;
    }
    return *arg0;
}
