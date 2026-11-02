#include "include_asm.h"
typedef int s32;
typedef unsigned char u8;
typedef short s16;
typedef float f32;
extern s32 D_L11_001B1530 __attribute__((sda));
extern s32 D_L11_001B1540 __attribute__((sda));
extern s32 D_L11_001B1550 __attribute__((sda));
extern s32 D_L11_001B1560 __attribute__((sda));
extern u8 D_L11_001BA8C0[];
extern u8 *D_L11_001B359C[3] __attribute__((sda));
extern u8 *D_L11_001FC420[];
extern s32 D_L11_001B35B8 __attribute__((sda));
extern u8 *D_L11_001B359C_abs[3] __attribute__((sda));
extern s32 func_L11_00442B60(u8 *);
extern s32 func_L02_003D77B8(u8 *);
extern f32 D_L11_001B15AC __attribute__((sda));
extern f32 D_L11_001B15B0 __attribute__((sda));
extern f32 D_L11_001B15B4 __attribute__((sda));
extern f32 D_L11_001B15B8 __attribute__((sda));
extern float func_00283B98(float, float);
extern float func_00283180(float);
extern float func_00283198(float);
extern float func_002A7878(float, float);
extern void func_00282CC0(u8 *, u8 *, u8 *);
extern void func_002833F0(u8 *, u8 *);
extern u8 *func_L11_004417E8(u8 *);
extern void func_L11_00436910(char *, int, int);
extern void func_L11_004496A0(char *, int, int);
extern void func_L11_0044C328(char *, int, int);
extern void func_L11_00452C58(char *, int, int);
extern void func_L11_00454C78(char *, int, int);
extern s32 D_L11_001B35B8_1[] __attribute__((sda));
extern f32 D_L11_001B1614 __attribute__((sda));
extern f32 D_L11_001B1618 __attribute__((sda));
extern s32 func_00312B58(u8 *);
extern s32 func_L02_00416800(u8 *);
extern s32 func_002A77E0(s32);

/* Turns the script's angle around the actor, hands the result to the level's helpers
   and counts the class the script's identifier names. */
s32 func_PAL_L11_004461C8(u8 *object, u8 *arg) {
    u8 *state = *(u8 **)(object + 0x68);
    f32 step = D_L11_001B15B4;
    f32 v;

    v = func_00283B98(*(f32 *)(state + 0x19C), step);
    *(f32 *)(state + 0x19C) = v;
    v = func_00283180(v);
    *(f32 *)(arg + 0x10) = D_L11_001B15B8 * v;
    v = func_00283198(*(f32 *)(state + 0x19C));
    *(f32 *)(arg + 0x14) = D_L11_001B15B8 * v;
    func_00282CC0(arg + 0x10, arg + 0x10, func_L11_004417E8(object));
    *(f32 *)(arg + 0x18) = func_002A7878(D_L11_001B15AC, D_L11_001B15B0);
    *(f32 *)(arg + 0xF4) = -1.570796489715576171875f;
    *(s32 *)(arg + 0xF0) = 0;
    *(f32 *)(arg + 0xF8) = *(f32 *)(state + 0x19C);
    func_002833F0(arg + 0xC0, arg + 0xF0);
    switch (*(s16 *)(arg + 0xAA)) {
    case 0xD98:
        func_L11_004496A0(arg, 0, 1);
        *(s32 *)(state + 0x100) += 1;
        break;
    case 0xDCF:
        func_L11_0044C328(arg, 0, 1);
        *(s32 *)(state + 0x104) += 1;
        break;
    case 0xDF4:
        func_L11_00452C58(arg, 0, 1);
        *(s32 *)(state + 0x108) += 1;
        break;
    case 0xDF7:
        func_L11_00454C78(arg, 0, 1);
        *(s32 *)(state + 0x10C) += 1;
        break;
    case 0xD37:
        func_L11_00436910(arg, 0, 1);
        *(s32 *)(state + 0x114) += 1;
        break;
    }
    return 1;
}
