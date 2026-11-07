#include "common.h"

extern s32 D_00167F5C_[];
extern s32 D_00167F60_[];
#define D_00167F5C (D_00167F5C_[0])
#define D_00167F60 (D_00167F60_[0])

s32 func_01EA0640(s32 arg0, s32 arg1) {
    D_00167F5C = arg0;
    D_00167F60 = arg1;
    return 0;
}
