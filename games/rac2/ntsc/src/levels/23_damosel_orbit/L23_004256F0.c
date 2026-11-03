#include "include_asm.h"

/* func_L23_004256F0 is a unit of its own: it needs the opt-in quadword-copy profile and its
   scheduling barrier (-frac2-qcopy -frac2-qcopy-barrier, config/cflags.txt), which give
   retail's grouped `lq`/`sq` with register addresses and keep the instructions around the
   copy on their own side of it. */

typedef unsigned char u8;

typedef int s32;

typedef int TI16 __attribute__((mode(TI)));

extern void *func_0029EFD0(int size);

extern float func_002A7940(void);

/* allocates the record, fills its sub-record from the arguments, then draws three samples into it */
void *func_L23_004256F0(u8 *obj, u8 *a1, u8 *a2, u8 a3, u8 a4, short a5, short a6, s32 a7, float f1, float f2,
                        float f3, float f4, s32 a9, s32 a10, s32 a11, s32 a12) {
    u8 *s0;
    u8 *v1;

    s0 = func_0029EFD0(0x11AC);
    if (s0 != 0) {
        v1 = *(u8 **)(s0 + 0x68);
        v1[0x28] = a3;
        v1[0x29] = a4;
        *(float *)(v1 + 0x10) = f1;
        *(float *)(v1 + 0x14) = f2;
        *(float *)(v1 + 0x18) = f3;
        *(float *)(v1 + 0x1C) = f4;
        *(short *)(v1 + 0xC) = a5;
        *(short *)(v1 + 0xE) = a6;
        *(s32 *)v1 = a7;
        *(s32 *)(v1 + 4) = a9;
        v1[0x2A] = a10;
        *(s32 *)(v1 + 0x2C) = a11;
        v1[0x2B] = a12;
        s0[0x30] = 0xFF;
        *(u8 **)(s0 + 0xB8) = a1;
        s0[0x20] = 0;
        *(u8 **)(v1 + 0x20) = a2;
        *(u8 **)(v1 + 0x24) = obj;
        *(TI16 *)(s0 + 0x10) = *(TI16 *)obj;
        *(float *)(s0 + 0xF0) = func_002A7940();
        *(float *)(s0 + 0xF4) = func_002A7940();
        *(float *)(s0 + 0xF8) = func_002A7940();
        s0[0x20] = 0;
    }
    return s0;
}
