#include "include_asm.h"

#include "common.h"
extern void func_00292408();
extern void func_L00_002FE268();
extern void func_002934B8();
extern u8 D_L08_001DB5E7;
extern s32 D_L08_001DAC40[];
extern s32 D_L08_001BFEF4;
extern u8 *D_L08_001B29DC __attribute__((sda));
extern u8 *D_L08_001B29E0 __attribute__((sda));
extern u8 *D_L08_001B29DC __attribute__((sda));
extern u8 *D_0018C0B0;

void func_PAL_L08_0030C3A8(s32 x) {
    func_00292408(x);
    func_L00_002FE268(x);
    func_002934B8(D_L08_001DAC40[D_L08_001DB5E7], D_L08_001BFEF4);
}

