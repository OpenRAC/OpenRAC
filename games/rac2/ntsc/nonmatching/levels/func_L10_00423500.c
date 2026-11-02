/* Sol low tier; 156/156 bytes, 0/1 places. Current pinned toolchain, G8.
   Four words differ at +0x50..+0x60: record address/result registers v0/a1 versus retail a0/v1.
   First-call scheduling now matches after moving the +0x14 zero store before the call.
   Corrected GP symbol names to GP+0x23B0/0x23B4/0x2448. Declarations included below. */
#include "common.h"



extern s32 D_L10_001B13A0 __attribute__((sda));
extern s32 D_L10_001B13A4 __attribute__((sda));
extern s32 D_L10_001B1438 __attribute__((sda));
extern u8 D_L10_00221CD0[];
typedef struct { s32 pad[4]; s32 a, b; s32 rest[4]; } ResetRecord;
extern ResetRecord D_L10_002A3C70[];

extern s32 func_0037C8B0(s32 a0);
extern void func_0037CFD8(u8 *a0);
extern void func_0037BD10(u8 *a0);
extern void func_0037F490(void);
extern void func_0037BD50(void);

void func_L10_00423500(void) {
    u8 *s0 = D_L10_00221CD0;
    ResetRecord *p;
    s32 r;

    D_L10_001B13A0 = -2;
    D_L10_001B1438 = 1;
    *(s32 *)(s0 + 0x2C) = 1;
    *(s32 *)(s0 + 0x28) = 0x64;
    D_L10_001B13A4 = 0;
    *(s32 *)(s0 + 0x14) = 0;
    r = func_0037C8B0(6);
    p = &D_L10_002A3C70[r];
    p->b = p->a;
    func_0037CFD8((u8 *)p);
    *(f32 *)(s0 + 0x30) = 8.0f;
    *(s32 *)(s0 + 0x10) = 0;
    func_0037BD10(0);
    func_0037F490();
    func_0037BD50();
}
