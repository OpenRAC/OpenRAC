/* Alternative reconstruction retained from PR #361. Not integrated or qualified;
 * compare with the primary near miss before reuse. */
/* func_L22_003F5688 (L22, 620 bytes, 2 places): NOT matched. Compiles to 616 bytes in the -G8 -frac2-qcopy
 * unit L22_003A5018-g8.c (only the first place was checked). Loop structure, the float constants,
 * the por/sq zeroing, the call chain and the gp/absolute split of the D_L22_001B0A.. words
 * (gp scalars; absolute reads via incomplete sda arrays indexed [0]) are right. Remaining
 * difference: gcc saves &v10 (sp+0x10) in a callee-saved register (frame 0xD0 instead of 0xC0, one
 * extra s-register) while retail rebuilds `addiu $a0,$sp,0x10` at each use; the register
 * numbering then shifts (a0 in s7 instead of s6). Tried: v10 as f32[4], as TI128 scalar (changes
 * the stack layout) and as TI128[1]; k=2.7f hoisted into a local (needed for $f26). Place in
 * src/levels/shared/L22_003A5018-g8.c. */
#include "common.h"

typedef int TI128 __attribute__((mode(TI)));

extern f32 D_L22_001B0A20 __attribute__((sda));
extern s32 D_L22_001B0A28 __attribute__((sda));
extern s32 D_L22_001B0A2C[] __attribute__((sda));
extern s32 D_L22_001B0A30[] __attribute__((sda));
extern s32 D_L22_001B0A34[] __attribute__((sda));
extern f32 D_L22_001B0A38[3] __attribute__((sda));
extern s32 D_L22_001B0A44 __attribute__((sda));
extern s32 D_L22_001B0A48 __attribute__((sda));
extern s32 D_L22_001B0A4C[] __attribute__((sda));
extern s32 D_L22_001B0A50[] __attribute__((sda));

extern f32 func_002F4BD0(s32 a);
extern f32 func_0031FD80(f32 a, f32 b);
extern f32 func_0031FD20(f32 a, f32 b);
extern void func_002F3F40(f32 *a, f32 *b, void *c);
extern void func_002F3A80(f32 *a, f32 *b, void *c);
extern s32 func_002F4C28(f32 a, s32 b, s32 c);
extern s32 func_0031FC88(s32 a);
extern s32 func_0031FCC8(s32 a, s32 b);
extern void func_002F3A48(f32 *a);
extern void func_00334950(f32 *a, f32 *b, f32 *c, s32 d, s32 e, s32 f, s32 g, s32 h, s32 i);

void func_L22_003F5688(u8 *a0, s32 a1)
{
    u8 *st = *(u8 **)(a0 + 0x68);
    f32 k = 2.7f;
    f32 total;
    f32 base;
    f32 fi;
    f32 r;
    f32 w;
    TI128 q10[1] __attribute__((aligned(16)));
    f32 v20[4] __attribute__((aligned(16)));
    f32 v30[4] __attribute__((aligned(16)));
    s32 i;
    s32 s2;
    s32 s1;
    s32 s0;

    total = *(f32 *)(st + 0xA0) * 7.2900004386901855f * D_L22_001B0A20;
    if (a1 == 0) {
        total = total * 0.2f;
    }
    base = *(f32 *)(st + 0xA0) / total;
    for (i = 0; (fi = (f32)i) < total;) {
        r = func_002F4BD0(i++) * base;
        *(TI128 *)v20 = 0;
        w = k + r * 0.1f;
        v20[0] = func_0031FD80(fi, w);
        v20[1] = func_0031FD80(fi, w);
        v20[2] = r;
        q10[0] = *(TI128 *)v20;
        func_002F3F40((f32 *)q10, (f32 *)q10, a0 + 0xC0);
        func_002F3A80((f32 *)q10, (f32 *)q10, a0 + 0x10);
        s2 = func_002F4C28(func_0031FD20(fi, 1.0f), D_L22_001B0A28, D_L22_001B0A2C[0]);
        s1 = func_002F4C28(func_0031FD20(fi, 1.0f), D_L22_001B0A30[0], D_L22_001B0A34[0]);
        w = D_L22_001B0A38[func_0031FC88(2)];
        func_002F3A48(v20);
        v20[3] = w;
        func_002F3A48(v30);
        v30[3] = w;
        s0 = func_0031FCC8(1, D_L22_001B0A48);
        func_00334950((f32 *)q10, v20, v30, s2, s1, s0, func_0031FCC8(D_L22_001B0A4C[0], D_L22_001B0A50[0]), 0x3C, D_L22_001B0A44);
    }
}
