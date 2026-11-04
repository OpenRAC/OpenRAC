#include "include_asm.h"

/* func_002BB838 (236 bytes, 28 places).
 *
 * The old draft was retail's body but for the 4.0f of func_00283CF0: the
 * reload hoisted the constant above the 16-byte copy.  The qcopy barrier
 * keeps the copy first, in retail's order.  Matches 28/28 in USA v1.01,
 * PAL and USA v2.00.
 */
extern void *func_002B9030(int kind);
extern int func_00283CF0(float f);
extern float func_00283CE0(int n);
extern unsigned char *D_001F2560;

typedef int TI128_002BB838 __attribute__((mode(TI)));

void func_002BB838(void *a0, void *a1, int a2, int a3, int a4, float f)
{
    void *p;
    char *q;
    int v;
    float w;

    if (a4 == 0)
        return;
    p = func_002B9030(8);
    if (p == 0)
        return;
    *(TI128_002BB838 *)((char *)p + 0x10) = *(TI128_002BB838 *)a0;
    *(int *)((char *)p + 4) = a2;
    v = func_00283CF0(4.0f);
    q = (char *)p + 0x20;
    *(unsigned char *)((char *)p + 9) = (unsigned char)(v - 0x60);
    *(unsigned char *)((char *)p + 3) = 0x48;
    *(unsigned char *)((char *)p + 1) = 0;
    *(int *)((char *)p + 0xC) = 0;
    *(unsigned char *)((char *)p + 8) = 0;
    *(unsigned char *)((char *)p + 2) = *D_001F2560;
    *(TI128_002BB838 *)q = *(TI128_002BB838 *)a1;
    *(short *)((char *)p + 0xA) = a4;
    *(int *)(q + 0x18) = a4;
    w = f / func_00283CE0(a4);
    *(int *)(q + 0x14) = a3;
    *(int *)(q + 0x10) = a2;
    *(float *)(q + 0x1C) = w;
}
