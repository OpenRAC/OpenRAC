/* func_L00_002B5A88 in its own unit (flags in config/cflags.txt: -O2 -G0 -ffunction-sections -frac2-qcopy -frac2-qcopy-barrier). */
#include "include_asm.h"

extern int func_L00_002B55F0_1(int);

typedef unsigned char u8;
typedef float f32;
typedef int V4_Fa012328c __attribute__((mode(TI)));

typedef struct { f32 x, y, z, w; } V4_F6b9e759f;

typedef struct {
    u8 pad[0x23C4];
    int i23C4;
    int i23C8;
} Resident;

extern Resident D_00189E20;
extern V4_F6b9e759f D_00189EA0_1[];

void func_L00_002B5A88(void)
{
    V4_F6b9e759f *tbl = D_00189EA0_1;
    V4_F6b9e759f *ring = tbl + 465;
    int i;

    i = 0;
    do {
        if (func_L00_002B55F0_1(i) == 1)
            break;
        if (D_00189E20.i23C8 >= 2) {
            D_00189E20.i23C8 = D_00189E20.i23C8 - 1;
            D_00189E20.i23C4 = (D_00189E20.i23C4 + 31) % 32;
        }
        *(V4_Fa012328c *)tbl = *(V4_Fa012328c *)&ring[D_00189E20.i23C4];
        i++;
    } while (i < 16);
}
