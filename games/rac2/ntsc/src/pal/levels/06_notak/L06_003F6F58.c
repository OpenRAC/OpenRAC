#include "include_asm.h"

extern int func_002A9640(char *a0, int a1, int a2);
extern float func_002A96A0(char *a0, int a1);
extern void func_002AB800(char *a0, char *a1);
extern void func_002A6C58(char *a0, int a1, int a2);
extern void func_002AB8C0(char *a0, char *a1);

/* runs a moby's timer down and picks the dying or the revived path from what is left */
void func_PAL_L06_003F6F58(char *a0, char *a1)
{
    float t = func_002A96A0(a0, func_002A9640(a0, 0xC30000, 0));

    if (t > 0.0f) {
        float left = *(float *)(a1 + 0x20) - t;

        *(float *)(a1 + 0x20) = left;
        if (left > 0.0f) {
            *(unsigned char *)(a1 + 0x57) = 0x5A;
            func_002AB800(a0, a1 + 0x50);
        } else {
            func_002A6C58(a0, 0, 3);
            *(unsigned char *)(a0 + 0x20) = 2;
            *(unsigned char *)(a1 + 0x57) = 0x78;
            func_002AB800(a0, a1 + 0x50);
        }
    }
    *(unsigned char *)(a0 + 0xA8) = 0xFF;
    func_002AB8C0(a0, a1 + 0x50);
}

