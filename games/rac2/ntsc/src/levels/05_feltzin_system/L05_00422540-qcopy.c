#include "common.h"
typedef int TI128 __attribute__((mode(TI)));

extern u8 *func_0037B440(s32 a0, s32 a1);
extern s32 func_00332AF8(s32 a0, s32 a1);
extern void func_00329960(u8 *a0, s32 a1, u8 *a2);
extern void func_003298C0(u8 *a0, s32 a1, u8 *a2);
extern void func_00306748(u8 *a0, u8 *a1, u8 *a2);
extern void func_003068F8(u8 *a0, u8 *a1, f32 f12);
extern void func_0037D210(u8 *a0);

void func_L05_00422540(u8 *arg0) {
    TI128 v[6];
    u8 *pw;
    u8 *px;
    u8 *p;
    s32 t;

    p = func_0037B440(0x10F5, 1);
    *(TI128 *)&v[0] = 0;
    ((f32 *)&v[0])[2] = 1.0f;
    if (p == 0)
        return;
    t = func_00332AF8(0, 4);
    pw = (u8 *)&v[1];
    func_00329960(arg0, t, pw);
    px = (u8 *)&v[2];
    func_003298C0(arg0, t, px);
    qcopy(p + 0x10, pw);
    qcopy(p + 0xC0, px);
    func_00306748(p + 0xD0, p + 0xC0, (u8 *)&v[0]);
    func_003068F8(p + 0xD0, p + 0xD0, 1.0f);
    func_00306748(p + 0xE0, p + 0xD0, p + 0xC0);
    func_0037D210(p);
}
