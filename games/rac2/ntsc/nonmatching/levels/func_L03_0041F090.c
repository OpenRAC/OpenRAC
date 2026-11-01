/* ABI-correct alternative to the existing byte-matched C body; not linked.
 * -O2 -G8 -ffunction-sections: 228/228 bytes, 0/1 USA v1.01 and USA v2.00;
 * this canonical name is absent from PAL. The explicit entity argument adds
 * a redundant a0 copy at +0x38, where retail has a nop. A measured
 * -fcse-follow-jumps probe ties the default. The existing matched source
 * relies on the incoming argument register surviving its unprototyped call;
 * it is not a qualified native interface. Reopen with independent reaching-
 * value/control-flow evidence for this hard-register copy, without a pin,
 * false prototype or artificial barrier. No matching credit is added here.
 */
#include "common.h"
extern f32 D_L03_001B1C00 __attribute__((sda));
extern f32 D_L03_001B1C04 __attribute__((sda));
extern s32 func_002B38B8(u8 *);
extern void func_002AB650(u8 *);
extern void func_L00_003294F0(u8 *, int);

s32 func_L03_0041F090(u8 *entity) {
    u8 *state = *(u8 **)(entity + 0x68);
    f32 f0;
    f32 f1;
    s32 v0;

    if (*(s32 *)(state + 0x284) == -1) return 0;
    if (entity[0x20] != 9) return 0;

    if (func_002B38B8(entity) == 0) {
        entity[0xA8] = 0xFF;
        f1 = D_L03_001B1C00 * 0.00027777778450399637f;
        f0 = D_L03_001B1C04 * 0.016666668f;
        v0 = *(s16 *)(state + 0x24);
        *(u16 *)(state + 0x98) = 0;
        *(f32 *)(state + 0x138) = f0;
        *(f32 *)(state + 0x33C) = 6.0f;
        *(f32 *)(state + 0x134) = f1;
        *(f32 *)(state + 0x130) = f1;
        *(f32 *)(state + 0x20) = (f32)v0;
        entity[0xA8] = 0xFF;
        *(u32 *)(entity + 0x98) = *(u32 *)(*(u8 **)(entity + 0x24) + 0x10);
        func_002AB650(entity);
        func_L00_003294F0(entity, 1);
        return 1;
    }
    return 0;
}
