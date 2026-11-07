#include "common.h"

extern s32 D_00169BBC_[];
extern s32 D_00169C88_[];
extern s32 D_00169C94_[];
#define D_00169BBC (D_00169BBC_[0])
#define D_00169C88 (D_00169C88_[0])
#define D_00169C94 (D_00169C94_[0])

s32 func_01ED4760(s32 arg0, s32 arg1) {
    D_00169C94 = arg0;
    D_00169C88 = arg1;
    D_00169BBC = 3;
    return 0;
}
