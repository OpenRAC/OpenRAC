#include "include_asm.h"
typedef unsigned char u8;
typedef short s16;
typedef int s32;
extern u8 *D_L23_001B1484 __attribute__((sda));
extern u8 *D_L23_001B1484_2;
extern s32 D_L23_001B1958 __attribute__((sda));
extern s32 D_L23_001B1954 __attribute__((sda));
extern s32 D_0018C0B0 __attribute__((nosda));
extern s32 D_L23_001B1430 __attribute__((sda));
extern s16 D_L23_001B1B94 __attribute__((sda));
extern s16 D_L23_001B1B96 __attribute__((sda));
extern void func_00291450(s32);
extern void func_L00_002E6CF0(void *, void *);
extern void func_L23_00429DC8(void *);
extern void func_L23_00429FF8(void *);
extern void func_L23_0042A8C0(void *);
extern void func_L23_0042A018(void *);
extern void func_L23_0042A438(void *);
extern s32 func_L23_0041B948(void);
extern s32 func_L23_0041C0C0(void);
extern s32 func_L23_0041C0B0(void);
extern float D_L23_001B3930 __attribute__((sda));
extern s32 D_L23_001B194C __attribute__((sda));
extern s32 D_L23_001B1950 __attribute__((sda));
extern s32 D_L23_001A8AB0[4] __attribute__((sda));
extern s32 D_L23_001A8AC0[4] __attribute__((sda));
extern s32 D_L23_001B3714;
extern s32 D_L23_001B13E8 __attribute__((sda));
extern s32 func_002A77E0(s32);
typedef int TI16 __attribute__((mode(TI)));
extern s32 D_L23_001A8AB4[3] __attribute__((sda));
extern s32 D_L23_001A8AC4[3] __attribute__((sda));
extern s32 D_L23_001A8FF0;
extern s16 func_00291308(TI16 *, s32, float, float);
extern u8 *func_0029EFD0(s32);
extern s32 func_L00_0035F5A0(s32, s32, s32);
extern s32 D_L23_001B19C4 __attribute__((sda));
extern s32 D_L23_001B19C8 __attribute__((sda));
extern s32 D_L23_001B19D8 __attribute__((sda));

/* releases the measured handles and seeds the frame */
void func_PAL_L23_0042F208(u8 *arg0) {
    s32 *payload = *(s32 **)(arg0 + 0x68);

    func_L23_00429DC8(arg0);
    if (D_L23_001B1484_2 != 0) {
        if (func_L23_0041B948() != 0) {
            func_L00_002E6CF0((void *)func_L23_0042A018, arg0);
        } else if (func_L23_0041C0C0() != 0) {
            func_L00_002E6CF0((void *)func_L23_0042A438, arg0);
        } else if (func_L23_0041C0B0() != 0) {
            func_L23_0042A8C0(arg0);
        } else {
            payload[1] = 0;
        }
        if (func_L23_0041C0C0() == 0) {
            s16 handle = D_L23_001B1B94;

            if (handle != -1) {
                D_L23_001B3930 = 0.02f;
                func_00291450(handle);
                D_L23_001B1B94 = -1;
            }
        }
        if (func_L23_0041C0B0() == 0) {
            s16 handle = D_L23_001B1B96;

            if (handle != -1) {
                func_00291450(handle);
                D_L23_001B1B96 = -1;
            }
        }
    }
    func_L23_00429FF8(arg0);
}
