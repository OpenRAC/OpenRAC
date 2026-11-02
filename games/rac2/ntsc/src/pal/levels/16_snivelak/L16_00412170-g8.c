#include "include_asm.h"
typedef unsigned char u8;
typedef signed int s32;
extern void func_002A7750(void *, s32, s32);
extern s32 D_L16_001B18F0 __attribute__((sda));
extern s32 D_L16_001B18E0 __attribute__((sda));
extern s32 D_L16_001B1704 __attribute__((sda));
extern char D_L16_00189E20[] __attribute__((nosda));
extern float D_L16_001B1708 __attribute__((sda));
extern float D_L16_00189EF8 __attribute__((nosda));
extern void func_002A7470(void *, s32, s32, void *);
extern s32 func_00283CF0(float);
extern s32 func_002A9640(void *, s32, s32);
extern s32 func_00282978(void *);
extern s32 func_002B3BA8(void *, s32, float, float, s32, s32);
extern s32 func_002A7820(s32, s32);

/* func_L16_00413EC8 is defined in L16_00403900.c (this unit has no $gp access for it). */

/* Two timed transitions of the same handle: the first drops the swung state once the
   measured clock passes the level's threshold, the second raises it below the other. */
void func_PAL_L16_00415CD0(u8 *arg0)
{
    u8 *s0;
    u8 *q;

    s0 = *(u8 **)(arg0 + 0x68);
    if (*(s32 *)(s0 + 0x26C) != 0) {
        if (*(float *)(s0 + 0x264) + D_L16_001B1708 > D_L16_00189EF8) {
            *(s32 *)(s0 + 0x26C) = 0;
            *(s32 *)(s0 + 0x270) = *(u8 *)(arg0 + 0x20);
            *(s32 *)(s0 + 0x274) = *(u8 *)(arg0 + 0x95);
            func_002A7750(arg0, 0x10, 4);
        }
    } else {
        q = D_L16_00189E20;
        if (*(s32 *)(q + 0x2294) != 0x81
            && *(float *)(s0 + 0x264) + D_L16_001B1708 < *(float *)(q + 0xD8)) {
            *(s32 *)(s0 + 0x268) = 0;
            *(s32 *)(s0 + 0x26C) = 1;
            if (*(u8 *)(arg0 + 0x43) != 0x16)
                func_002A7470(arg0, 0x16, 0, 0x19);
            if (*(u8 *)(*(u8 **)(s0 + 0x1F0) + 0x43) == 0xA)
                func_002A7470(*(u8 **)(s0 + 0x1F0), 0, 0, 0x19);
        }
    }
}
