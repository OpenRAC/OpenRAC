/* cflags: -mno-split-addresses */
#include "common.h"

s32 func_01EEA034(s32, s32);
s32 func_01F087F8(s32, s32);

void func_01F08888(s32 arg0) {
    func_01F087F8(arg0, 0);
    func_01EEA034(arg0 + 0xB8, 1);
}
