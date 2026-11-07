/* cflags: -mno-split-addresses */
#include "common.h"

extern s32 D_0021DE10_[];
#define D_0021DE10 (D_0021DE10_[0])

s32 func_00510EB0(void) {
    return (u32) (D_0021DE10 - 0x15) < 0xFU;
}
