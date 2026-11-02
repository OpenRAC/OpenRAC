#include "include_asm.h"

extern int D_L00_001BCAF4;
extern unsigned char D_001A7BB1 __attribute__((sda));
extern float func_00283180(float);
extern int func_00283D38(float, int, int);
extern unsigned char *func_00288FB8(int);
extern int func_0027FB40(int, int, long, unsigned char *, int);

void func_PAL_0027D9A0(void)
{
    int y;
    long style;
    unsigned char *text;
    float f;
    int v;

    y = 0x148;
    if (D_001A7BB1) {
        y = 0x17C;
    }
    v = D_L00_001BCAF4;
    f = (float)(v % 100) * 0.02f - 1.0f;
    if (f < -1.0f) {
        f = -1.0f;
    }
    if (1.0f < f) {
        f = 1.0f;
    }
    f = func_00283180(f * 3.14159274f);
    f = f + 1.0f;
    style = func_00283D38(f * 0.5f, 0x7FE0E0E0, 0x5FF0C070);
    text = func_00288FB8(0xE45);
    func_0027F5B8(0x3C, y, style, text, -1);
}

