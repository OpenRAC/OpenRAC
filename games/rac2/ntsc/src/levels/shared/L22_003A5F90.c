#include "include_asm.h"

typedef unsigned char u8;
typedef float f32;

extern int func_002A77E0(int);
extern int D_L22_001B2AA8;
extern int *D_L22_001B2AB4;

void func_L22_003A5F90(u8 *obj)
{
    u8 *p = *(u8 **)(obj + 0x68);
    int i = func_002A77E0(3);
    int n = D_L22_001B2AA8;
    int *w;
    int m;
    int first;

    if ((f32)(n / 6) >= *(f32 *)(p + 0x20)) {
        i = 0;
    }
    w = D_L22_001B2AB4;
    m = n;
    first = i;
    do {
        if (i == 0) {
            if (*(f32 *)(p + 0x20) < (f32)(m / 3)) {
                *(f32 *)(p + 0x20) = *(f32 *)(p + 0x20) + (f32)*(short *)(p + 0x322);
                return;
            }
        } else if (i == 1) {
            if (w[2] < *(short *)(p + 0x32A)) {
                w[2] = w[2] + *(short *)(p + 0x32A) / 2;
                if (w[2] > *(short *)(p + 0x32A)) {
                    w[2] = *(short *)(p + 0x32A);
                }
                return;
            }
        } else if (i == 2) {
            if (w[3] < *(short *)(p + 0x32E)) {
                w[3] = w[3] + *(short *)(p + 0x32E) / 2;
                if (w[3] > *(short *)(p + 0x32E)) {
                    w[3] = *(short *)(p + 0x32E);
                }
                return;
            }
        }
        i = (i + 1) % 3;
    } while (i != first);
}

