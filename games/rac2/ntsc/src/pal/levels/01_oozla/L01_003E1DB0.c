#include "include_asm.h"

#include "common.h"
extern s32 func_L01_00321178(void *, void *);
extern s32 func_310648();
extern f32 func_310580(f32 x, f32 y);
extern s32 func_310528(s32 a, s32 b);
extern f32 func_2E3A48(f32 x);
extern f32 func_2E3A60(f32 x);
extern s32 func_00277F48(f32 radius, void *pos, s32 flag, void *obj, s32 arg);
extern u32 D_L01_001C3980[];
extern void func_002AB8C0(void *, void *);
extern void func_00282978(void *);
extern s32 func_00282948(void *);
extern void func_002B0990(void *, void *, s32, s32, f32);
extern u8 *D_L01_001B1F04 __attribute__((sda));
extern s32 func_L00_004290C0(void *);

void func_PAL_L01_003E1DB0(u8 *a0)
{
    u32 v;
    u8 *p = *(u8 **)(a0 + 0x68);

    func_002AB8C0(a0, p + 0x100);
    v = *(u16 *)(a0 + 0x96);
    if (v <= 0xFFFE) {
        *(u16 *)(a0 + 0x96) = v + 1;
    }
    func_00282978(p + 0x3D8);
    func_00282948(p + 0x404);
    func_00282978(p + 0x3D6);
    v = *(s16 *)(p + 0x3DA);
    if (v != 0) {
        *(u16 *)(p + 0x3D6) = 0x12C;
        *(s16 *)(p + 0x3DA) = 0;
    }
    func_002B0990(a0, p + 0x300, 1, 1, 1.8f);
}

