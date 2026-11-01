#include "common.h"

/* func_L10_00418768 needs -fstrict-aliasing (config/cflags.txt): retail loads the record
   table base D_001B251C ahead of the state reload at +0x144, which the scheduler may only
   do when the pointer load is known not to alias the int store to the record before it.
   `one` is measured: retail keeps 1 in $s0 across the func_0037D390 call. */

extern u8 *D_001B251C __attribute__((sda));
extern u8 D_L10_00188660[];

extern u8 *func_0037B670(u8 *arg0);
extern void func_L10_004189D8(u8 *arg0);
extern void func_00308AF8(u8 *a0);
extern void func_0037BC48(u8 *arg0, s32 a1, s32 a2, s32 a3);
extern u8 *func_0037BA88(u8 *arg0);
extern void func_0037DFB8(u8 *a0);
extern s32 func_0037D390(u8 *arg0, s32 a1, s32 a2, s32 a3);
extern void func_00373410(s32 a0);
extern s32 func_003737B8(s32 a, s32 b, u8 *c, s32 d);
extern void func_0037D7A0(u8 *a0, s32 a1, f32 f12);
extern void func_L10_00418AD8(u8 *arg0);
extern s32 func_0037D6E0(u8 *a0);
extern void func_0037D528(u8 *a0);
extern void func_003087A0(u8 *a0);
extern void func_L10_00418BA0(u8 *arg0);
extern void func_L10_00418968(u8 *arg0);

void func_L10_00418768(u8 *arg0) {
    u8 *s2 = *(u8 **)(arg0 + 0x68);
    u8 *s3;
    u8 *p;
    s32 id;
    s32 tmp[4];
    s32 one;

    if (arg0[0x20] == 0) {
        arg0[0x30] = 0xFF;
        if (func_0037B670(arg0) == 0)
            return;
        func_L10_004189D8(arg0);
        func_00308AF8((u8 *)tmp);
        if (*(s32 *)(s2 + 0x264) != 0)
            func_0037BC48(arg0, (s32)func_L10_00418968, 0, 0);
        return;
    }
    s3 = func_0037BA88(arg0);
    if (*(s32 *)(s3 + 0x110) == 0)
        return;
    func_0037DFB8(arg0);
    one = 1;
    if (func_0037D390(arg0, 0, 0, 0) == one) {
        id = *(s32 *)(s2 + 0x26C);
        if (id != -1) {
            p = D_L10_00188660 + id * 0x70;
            if (*(u8 **)(p + 0x88) == arg0 && p[0x74] != 0)
                func_00373410(id);
        }
        *(s32 *)(s2 + 0x26C) = -1;
        func_003737B8(2, 0, arg0, 0xF8E);
    }
    func_0037D7A0(arg0, 0, 176.0f);
    if (*(s32 *)(s3 + 0x110) == 1) {
        if (*(s32 *)(s2 + 0x260) != -1) {
            *(s32 *)(s3 + 0x11C) = *(s32 *)(s3 + 0x110);
            *(u8 **)(s3 + 0x120) = D_001B251C + (*(s32 *)(s2 + 0x260) << 8);
        }
        if (*(s32 *)(s3 + 0x104) != 0)
            func_L10_00418AD8(arg0);
        if (func_0037D6E0(arg0) != 0)
            func_0037D528(arg0);
        func_003087A0(s2 + 0x268);
        func_L10_00418BA0(arg0);
    }
    p = D_L10_00188660 + *(s32 *)(s2 + 0x26C) * 0x70;
    if (*(u8 **)(p + 0x88) != arg0 || p[0x74] == 0)
        *(s32 *)(s2 + 0x26C) = func_003737B8(0, 6, arg0, 0xF8E);
}
