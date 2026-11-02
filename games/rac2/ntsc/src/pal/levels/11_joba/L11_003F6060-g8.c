#include "include_asm.h"
#include "common.h"
extern void func_002A7750(u8 *arg0, s32 arg1, s32 arg2);
extern s32 func_002A77E0(s32 arg0);
extern s32 func_L11_00440C80(s32 arg0);
extern s32 D_L11_001B056C __attribute__((sda));
extern s32 D_L11_001B058C __attribute__((sda));
extern f32 D_L11_001B0670;
extern u8 *func_002A9640(s32 arg0, s32 arg1, s32 arg2);
extern f32 func_002A96A0(u8 *arg0, u8 *arg1);
extern void func_002AB800(u8 *arg0, u8 *arg1);
extern s32 func_002B3380(u8 *arg0, u8 *arg1, u8 *arg2);
extern s32 func_002B3BA8(u8 *arg0, s32 arg1, s32 arg2, s32 arg3, f32 arg4, f32 arg5);
extern void func_L11_003F3098(u8 *arg0);
extern f32 D_L11_001B0570 __attribute__((sda));
extern s32 D_L11_001FFA78[];
extern f32 func_00283248(f32 a0, f32 a1);
extern s32 func_00283CF0(f32 a0);
extern u32 D_L11_001B0D9C __attribute__((sda));
extern u8 D_00189E20[];

/* Runs the object's timer down, keeps the shared step counter in step with it, and retires
   the object when the timed effect is over. */
void func_PAL_L11_003F6060(u8 *arg0) {
    u8 *s0 = arg0;
    u8 *s1;
    u8 *s2;
    u8 *s3;
    u32 off;
    f32 f2;

    if (*(u8 *)(s0 + 0x20) == 0) {
        return;
    }
    s1 = *(u8 **)(s0 + 0x68);
    s2 = func_002A9640(s0, 0x10000, 0);
    f2 = func_002A96A0(s0, s2);
    if (s2 != 0) {
        if (*(u8 *)(s0 + 0x20) >= 3) {
            *(f32 *)(s1 + 0x20) = *(f32 *)(s1 + 0x20) - f2;
            /* The index is a byte offset into the table: keeping it in a variable leaves
               the constant out of the symbol's address, as the retail has it. */
            off = *(u8 *)(s1 + 0x37) * 4 + 0xE0;
            *(s32 *)((u8 *)D_L11_001FFA78 + off) += (s32)(f2 * 32.0f);
            if (*(f32 *)(s2 + 0x2C) != 0.0f) {
                s3 = s1 + 0x70;
                func_002AB800(s0, s3);
                if (*(f32 *)(s2 + 0x2C) >= D_L11_001B0570 || *(f32 *)(s1 + 0x20) <= 0.0f) {
                    func_002AB800(s0, s3);
                    func_002A7750(s0, 8, -1);
                }
            }
        }
    }
    *(u8 *)(s0 + 0xA8) = 0xFF;
    func_002B3380(s0, s1 + 0x50, s1 + 0x20);
    if (*(u8 *)(s0 + 0x20) >= 2 && *(u8 *)(s1 + 0x2E) == 2) {
        *(s32 *)(s1 + 0x20) = 0;
        func_L11_003F3098(s0);
    }
    *(s32 *)(s1 + 0x258) = func_002B3BA8(s0, *(s32 *)(s1 + 0x25C), 0, 0, 200.0f, 200.0f);
}
