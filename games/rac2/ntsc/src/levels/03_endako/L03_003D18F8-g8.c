#include "common.h"

extern char *func_3113F0(s32);
extern void func_32AA20(char *, char *);
extern f32 func_2EF670(f32, f32);
extern f32 func_2EF110(f32 *);
extern f32 D_L03_001B0648 __attribute__((sda));

char *func_L03_003D18F8(char *a0, char *a1, f32 *a2, s32 a3)
{
    char *s1 = func_3113F0(0xA02);
    char *s0;

    if (s1 != 0) {
        s0 = *(char **)(s1 + 0x68);
        func_32AA20(s1, a0);
        qcopy(s1 + 0x10, a1);
        qcopy(s0, a2);
        *(f32 *)(s1 + 0xF8) = func_2EF670(a2[0], a2[1]);
        {
            f32 t = func_2EF110(a2);
            f32 angle = -func_2EF670(t, a2[2]);
            f32 scale = D_L03_001B0648;

            *(f32 *)(s1 + 0xF4) = angle;
            *(s32 *)(s0 + 0x30) = 0x78;
            *(s32 *)(s0 + 0x38) = a3;
            *(f32 *)(s1 + 0x2C) = *(f32 *)(*(u32 *)(s1 + 0x24) + 0x24)
                * scale;
        }
    }
    return s1;
}
