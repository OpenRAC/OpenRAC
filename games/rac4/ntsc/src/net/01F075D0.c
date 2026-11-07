/* cflags: -mno-split-addresses */
#include "common.h"

s32 func_01F07170(void *, s32);
extern s32 D_1B7FA0_[];
#define D_1B7FA0 (D_1B7FA0_[0])

void func_01F075D0(void) {
    func_01F07170(&D_1B7FA0, 2);
}
