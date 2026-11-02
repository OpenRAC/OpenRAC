#include "include_asm.h"

typedef int s32;

typedef unsigned int u32;

typedef unsigned char u8;

typedef unsigned short u16;

typedef float f32;

/* The level's ripple state, read and written through $gp: a flag byte, the clock float
   the two frame functions share, and the two row indexes the frame walks.  Their
   addresses move with the level's data, so they are named by their role. */
extern s32 ref_L04_0039FEF0_00 __attribute__((sda));
extern f32 ref_L04_0039FEF0_01 __attribute__((sda));
extern s32 ref_L04_0039FEF0_02 __attribute__((sda));
extern s32 ref_L04_0039FEF0_03 __attribute__((sda));

extern void func_L04_0032F1D8(u32 a0, void *a1);
extern void func_L04_0032F62C(u32 a0, s32 a1, s32 a2, void *a3);

/* Walks the level's ripple patches, hands the marked ones to the two helpers and then
   swaps the two row indexes and rewinds the clock.  The flag value and the row stride
   are named so that the compiler keeps them in registers, as the retail does. */
void func_L04_0039FEF0(void *a0, s32 a1) {
    s32 i = 0;
    s32 old;

    if (a1 > 0) {
        f32 high = 0.35f;
        s32 one = 1;
        f32 low = 0.00625f;
        u32 stride = 0x5C0;
        u8 *p = (u8 *)a0;

        while (i < a1) {
            if (*(u16 *)(p + 0x1E) != 0) {
                if (*(u32 *)(p + 0x48) != 0) {
                    ref_L04_0039FEF0_00 = one;
                    ref_L04_0039FEF0_01 = high;
                } else {
                    ref_L04_0039FEF0_00 = 0;
                    ref_L04_0039FEF0_01 = low;
                }
                func_L04_0032F1D8(0x70000000, p + (ref_L04_0039FEF0_02 * stride + 0x50));
                func_L04_0032F62C(0x70000000, i, ref_L04_0039FEF0_03, a0);
            }
            i++;
            p += 0x1190;
        }
    }
    old = ref_L04_0039FEF0_02;
    ref_L04_0039FEF0_02 = ref_L04_0039FEF0_03;
    ref_L04_0039FEF0_03 = old;
    ref_L04_0039FEF0_01 = 0.00625f;
    ref_L04_0039FEF0_00 = 0;
}
