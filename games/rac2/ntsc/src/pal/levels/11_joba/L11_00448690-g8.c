#include "include_asm.h"
typedef int s32;
typedef unsigned char u8;
extern s32 D_L11_001B1530 __attribute__((sda));
extern s32 D_L11_001B1540 __attribute__((sda));
extern s32 D_L11_001B1550 __attribute__((sda));
extern s32 D_L11_001B1560 __attribute__((sda));
extern void func_3004A8(void *);
extern void func_4882F0(void *, void *, s32, s32, s32);
extern void func_488268(s32, s32);
extern void func_4881A8(void *);
extern void func_488200(void *);
extern u8 D_L11_001BA8C0[];
extern u8 *D_L11_001B359C[3] __attribute__((sda));
extern u8 *D_L11_001FC420[];
extern s32 D_L11_001B35B8 __attribute__((sda));
extern u8 *D_L11_001B359C_abs[3] __attribute__((sda));
extern s32 func_L11_00442B60(u8 *);
extern s32 func_L02_003D77B8(u8 *);
extern u8 *func_L11_004417E8(u8 *);
extern s32 D_L11_001B35B8_1[] __attribute__((sda));
extern s32 func_00312B58(u8 *);
extern s32 func_L02_00416800(u8 *);
extern s32 func_002A77E0(s32);

/* arms the script bridge over the measured buffers and starts it on the shared slot */
void func_PAL_L11_00448BD8(void) {
    s32 buffer[4];
    func_3004A8(buffer);
    func_4882F0(&D_L11_001B1550, &D_L11_001B1560, 1, 0, 0);
    func_488268(2, 0x12C);
    func_4881A8(&D_L11_001B1530);
    func_488200(&D_L11_001B1540);
}
