#include "include_asm.h"
#include "common.h"

extern char *D_L01_001B255C __attribute__((sda));

extern u16 *D_L01_001FB020[];

/* Raises the measured flag on every record of the table the object's selector names. */
void func_L01_003DC838(s32 a0) {
    s32 base;
    s16 *p;
    s16 *q;

    if (*(u8 *)(a0 + 0x21) == 0xFF)
        return;
    q = D_L01_001FB020[*(u8 *)(a0 + 0x21)];
    if (q == 0)
        return;
    p = q;
    base = (s32)D_L01_001B255C;
    do {
        u32 v = (*(u16 *)p & 0x7FFF) << 8;

        if (*(s8 *)(base + v + 0x20) >= 0) {
            if (*(s16 *)(base + v + 0xAA) == 0xB2B)
                *(u16 *)(*(u32 *)(base + v + 0x68) + 0x3DA) = 1;
        }
    } while (*p++ >= 0);
}

/* func_L01_00416A50 is defined in L01_003DA740.c (this unit has no $gp access for it). */

typedef unsigned int TI128 __attribute__((mode(TI)));

extern u8 *D_L01_001B264C __attribute__((sda));

extern char D_00189EA0[];

extern float func_00282E80(void *, void *);

extern float func_002A89A8(void *, int, float);

void func_L01_003DCE70(u8 *arg0, u8 *out) {
    s32 *p = (s32 *)(*(u32 *)(arg0 + 0x68) + 0x3C4);
    TI128 v;
    int count = 3;
    f32 f;

    *(f32 *)(out + 0xC) = 0.0f;
    while (--count >= 0) {
        s32 index = *p;

        if (index != -1) {
            u8 *base = D_L01_001B264C;

            qcopy_nc(&v, base + index * 128 + 0x30);
            f = func_00282E80(&v, D_00189EA0);
            if (*(f32 *)(out + 0xC) < f) {
                qcopy_nc(out, &v);
                *(f32 *)(out + 0xC) = f;
            }
        }
        p++;
    }
    *(f32 *)(out + 0x8) = func_002A89A8(out, 0, 10.0f);
}
