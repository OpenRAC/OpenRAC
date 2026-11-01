#include "include_asm.h"
typedef signed int s32;

extern s32 D_L08_001B39A0[];
extern s32 D_L08_001B39E0[];
extern s32 D_L08_001B39F0[];
extern s32 D_L08_001B3A30[];
extern void func_00291450(s32 a0);

/* Frees the one of the three slots whose 0x1B3A30 word still names A0. */
s32 func_L08_00429350(s32 a0)
{
    s32 *pE0, *pF0, *pA0, *p30;
    s32 i;
    s32 v;
    i = 0;
    pA0 = D_L08_001B39A0;
    pE0 = D_L08_001B39E0;
    pF0 = D_L08_001B39F0;
    p30 = D_L08_001B3A30;
    for (; i < 3; i++) {
        if (*p30 == a0) {
            v = *pE0;
            *pA0 = 0;
            *p30 = 0;
            if (v != -1) {
                func_00291450(v);
                *pE0 = -1;
            }
            v = *pF0;
            if (v != -1) {
                func_00291450(v);
                *pF0 = -1;
            }
            return 1;
        }
        pF0++;
        pE0++;
        pA0++;
        p30++;
    }
    return 0;
}

