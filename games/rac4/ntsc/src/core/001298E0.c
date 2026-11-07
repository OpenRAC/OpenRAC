#include "common.h"

s32 func_001268D0(s32, s32);
s32 func_001276D0(s32);
extern s32 D_00164698_[];
extern s32 D_1A6094_[];
#define D_00164698 (D_00164698_[0])
#define D_1A6094 (D_1A6094_[0])

void func_001298E0(void) {
    func_001276D0(5);
    func_001268D0(5, D_1A6094);
    D_00164698 = 0;
}
