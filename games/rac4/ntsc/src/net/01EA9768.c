#include "common.h"

extern s32 D_001681A4_[];
extern s32 D_001681AC_[];
#define D_001681A4 (D_001681A4_[0])
#define D_001681AC (D_001681AC_[0])

s32 func_01EA9768(s32 arg0, s32 arg1) {
    D_001681A4 = arg0;
    D_001681AC = arg1;
    return 0;
}
