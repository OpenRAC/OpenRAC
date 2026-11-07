#include "common.h"

s32 func_00490400(s32);
s32 func_004D3528();
extern s32 D_1EE600_[];
#define D_1EE600 (D_1EE600_[0])

void func_00490878(void) {
    void *var_s0;

    var_s0 = &D_1EE600;
    do {
        func_00490400((s32) var_s0);
        var_s0 += 0x5C0;
    } while ((s32) var_s0 < (s32) (&D_1EE600 + 0xB80));
    func_004D3528();
}
