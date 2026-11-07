#include "common.h"

s32 func_01EB8E98(s32 arg0, s32 arg1) {
    return (arg1 & 0x7FF) | ((arg0 & 0x1F) << 0xB);
}
