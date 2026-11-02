#include "include_asm.h"
extern void *D_0026CBB0[];
extern void *D_0026CBE0[];
extern int D_001A8BB0;
extern int D_001A8BB4;
extern int D_001A8BB8;
extern int D_0018C2EC __attribute__((nosda));
extern int D_001AA0F4 __attribute__((sda));
extern int D_001AA0F8 __attribute__((sda));
extern int D_001AA0FC __attribute__((sda));
extern char *D_001AA104 __attribute__((sda));
extern float D_001A8C7C;
extern float D_001B15A0;
extern int D_001B1694;
extern float D_001B90F0 __attribute__((nosda));
extern int D_001ABD80;
extern void func_002CA790(void);
extern void func_00285860(void);
extern void func_002F5EF8(void);
extern void func_002F57A8(void);
extern void func_0026FF08(int, int, float, float, float, float);
extern void func_0027AB40(void);
extern void func_002E7248(void);
extern void func_002F8768(void);
extern void func_002DF380(void);
extern void func_002FF288(void);
extern void func_00291128(void);
extern void func_00282A28(void);
extern void func_002B49B0(int);
extern int func_002857B0(void);
extern void func_002C9CF8(int);
extern void func_002858C0(int, int);
extern void func_002F5D40(int);
extern float func_002AA2E8(float *, float, float);
extern void func_002F53C8(int);
extern void func_00288CB0(int, int);
extern void func_002E71B8(void);
extern void func_002E7090(int);
extern void func_002E7018(void);
extern void func_002F8178(int);
extern void func_002DF2E0(int, int);
extern void func_002FF0D8(void);
extern void func_00291108(void);

/* Applies a requested change of mode: D_001A8BB8 holds the requested mode (-2 when none) and
   D_001A8BB0 the current one. The first dispatch runs the handler of the mode being left and the
   second the handler of the mode being entered; unless that one holds the change (hold), the
   request becomes the current mode and is cleared. Both dispatches read the tables retail has in
   its data (ten code addresses each, one per mode, D_0026CBB0 and D_0026CBE0); `leaving` and
   `entering` only keep the labels alive. A handler reached through a table starts with no known
   values, so D_001A8BB8 is read again where retail reads it (the compare in the first handler of
   mode 3) without any qualifier on the global. */
void func_US2_002B5B38(void)
{
    static void *leaving[12] = {&&L1_3, &&L1_4, &&L1_1, &&L1_2, &&L1_6, &&L1_5, &&L1_7, &&L1_8, &&L1_9, &&L1_0, &&L1_def, &&shared};
    static void *entering[11] = {&&L2_3, &&L2_0, &&L2_4, &&L2_1, &&L2_2, &&L2_6, &&L2_5, &&L2_7, &&L2_8, &&L2_9, &&L2_def};
    int mode = D_001A8BB0;
    int sel;
    int hold;
    D_001A8BB4 = mode;
    if (D_001A8BB8 == -2)
        return;
    if (D_001AA0F4 > 0) {
        D_001AA0F4--;
        return;
    }
    if (D_0018C2EC == 0)
        return;
    if ((unsigned)mode >= 10)
        goto L1_def;
    goto *D_0026CBB0[mode];
L1_3:
    if (D_001A8BB8 != 4 || D_001AA0F8 == 8)
        func_002CA790();
    goto E1;
L1_4:
    func_00285860();
    goto E1;
L1_1:
    func_002F5EF8();
    goto shared;
L1_2:
    func_002F57A8();
shared:
    D_001B90F0 = 0.62f;
    func_0026FF08(0, 3, 1.1243411f, 0.005f, 0.2f, 0.0f);
    func_0027AB40();
    goto E1;
L1_6:
    if (D_001AA0F8 != 4)
        func_002E7248();
    goto E1;
L1_5:
    func_002F8768();
    goto E1;
L1_7:
    func_002DF380();
    goto E1;
L1_8:
    func_002FF288();
    goto E1;
L1_9:
    func_00291128();
    goto E1;
L1_def:
    func_00282A28();
L1_0:
E1:
    func_002B49B0(10);
    hold = 0;
    sel = D_001A8BB8;
    if ((unsigned)sel >= 10)
        goto L2_def;
    goto *D_0026CBE0[sel];
L2_3:
    if (D_001A8BB4 != 4 || func_002857B0() == 8)
        func_002C9CF8(D_001AA0F8);
L2_0:
    hold = 0;
    goto E2;
L2_4:
    func_002858C0(D_001AA0F8, D_001AA0FC);
    goto E2;
L2_1:
    func_002F5D40(D_001AA0FC);
    hold = 0;
    goto E2;
L2_2:
    if ((unsigned)(D_001A8BB0 - 1) >= 2) {
        float f;
        func_002AA2E8(&D_001A8C7C, 1.2f, 0.10000001f);
        f = D_001A8C7C;
        D_001B15A0 = f;
        if (1.0f < f)
            D_001B15A0 = 1.0f;
        if (1.2f <= f) {
            hold = 0;
            D_001A8C7C = 1.0f;
            func_002F53C8(D_001AA0F8);
        } else {
            hold = 1;
        }
    } else {
        func_002F53C8(D_001AA0F8);
    }
    goto E2;
L2_6:
    if (D_001AA0F8 == 5) {
        func_00288CB0(D_001AA0FC, 1);
    } else if (D_001AA0F8 == 6) {
        func_002E71B8();
    } else {
        if (D_001A8BB4 == 3)
            func_002E7090(D_001AA0FC);
        else
            func_002E7018();
        D_001B1694 = 1;
    }
    goto E2;
L2_5:
    func_002F8178(D_001AA0F8);
    goto E2;
L2_7:
    {
        int fallback = D_001ABD80;
        func_002DF2E0(D_001AA0F8 ? D_001AA0F8 : fallback, D_001AA0FC);
    }
    goto E2;
L2_8:
    func_002FF0D8();
    goto E2;
L2_9:
    func_00291108();
    goto E2;
L2_def:
    func_00282A28();
E2:
    if (hold == 0) {
        int state;
        if (D_001AA104 != 0)
            *D_001AA104 = 1;
        state = D_001A8BB8;
        D_001A8BB8 = -2;
        D_001A8BB0 = state;
        D_001AA104 = 0;
    }
}
