#include "include_asm.h"
extern int func_002ACBC8(void *point, int index);
extern void func_00282CF0(float *out, const float *a, const float *b);
extern void func_00283098(float *out, const float *a, const float *b);
extern void func_L02_002DCEA0(int a0, float a1);
extern void func_L00_002B9D60(int a0, int a1, int a2, int a3);
extern char D_00189EA0[];
extern char D_L02_001BA500[];
extern char *D_L02_001B368C;
extern float D_L02_001B2FE0 __attribute__((sda));

/* Flags the actor's target when its record is on screen, and refreshes it afterwards. */
void func_PAL_L02_003D4340(char *arg0) {
    char *p = *(char **)(arg0 + 0x68);
    float v[4];

    if (*(int *)(p + 0xDC) == 0) {
        if (func_002ACBC8(D_00189EA0, *(int *)(p + 0xC4)) != 0) {
            func_00282CF0(v, (float *)D_L02_001BA500,
                          (float *)(D_L02_001B368C + (*(int *)(p + 0xC4) << 7) + 0x30));
            func_00283098(v, v, (float *)(D_L02_001B368C + (*(int *)(p + 0xC4) << 7) + 0x40));
            if (v[0] > 0.0f) {
                *(int *)(p + 0xDC) = 1;
                func_L02_002DCEA0(1, 10.0f);
            }
        }
    } else {
        if (D_L02_001B2FE0 >= 1.0f) {
            func_L00_002B9D60((int)D_00189EA0,
                              (int)(D_L02_001B368C + (*(int *)(p + 0xC4) << 7) + 0x70), 0, 1);
            *(short *)(D_00189EA0 + 0x160) = 8;
        }
    }
}
