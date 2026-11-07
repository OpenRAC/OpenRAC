#include "common.h"

s32 func_004FC518();
extern s16 D_00171B5C_[];
#define D_00171B5C (D_00171B5C_[0])

s32 func_004FC650(void) {
    return func_004FC518() - D_00171B5C;
}
