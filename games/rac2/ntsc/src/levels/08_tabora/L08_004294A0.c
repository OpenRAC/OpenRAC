#include "common.h"

extern s32 D_L08_001B39A0[] MACRO_ADDR;
extern s32 D_L08_001B3A30[] MACRO_ADDR;
extern char D_L08_001B3A40[] MACRO_ADDR;
extern char D_L08_002603E0[];

extern void func_00282B50(void *a, void *b, s32 c);

/* Transitions the slot matching the key (1 -> 2, 4 -> 3), copying the caller's
   transform and target vector into its per-slot storage. */
s32 func_L08_004294A0(s32 a0, void *a1, char *a2)
{
    s32 i;
    for (i = 0; i < 3; i++) {
        if (a0 == D_L08_001B3A30[i]) {
            s32 st = D_L08_001B39A0[i];
            if (st == 1) D_L08_001B39A0[i] = 2;
            else if (st == 4) D_L08_001B39A0[i] = 3;
            func_00282B50(D_L08_002603E0 + i * 0x40, a1, 0x40);
            qcopy(D_L08_001B3A40 + i * 0x10, a2);
            return 1;
        }
    }
    return 0;
}
