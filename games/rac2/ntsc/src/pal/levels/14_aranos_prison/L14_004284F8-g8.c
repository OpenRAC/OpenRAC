#include "include_asm.h"
extern float D_L14_001B16B0 __attribute__((sda));
extern int func_002A9640(char *a0, int a1, int a2);
extern void func_002B0ED8(char *a0, char *a1, int a2);
extern void func_002AC728(char *a0, char *a1, float f12, float f13, int a2);
extern void func_002A6C58(char *a0, int a1, int a2);
extern void func_0029F338(char *a0);

/* Resets the moby's scale when its model says so, then lets it fall or die. */
void func_PAL_L14_00428798(char *a0)
{
    char *s0 = a0;
    char *s1;
    char *m;
    char *v;

    s1 = *(char **)(s0 + 0x68);
    v = (char *)func_002A9640(a0, 0x10000, 1);
    if (v != 0) {
        m = *(char **)(v + 0x20);
        if (m != 0 && *(short *)(m + 0xAA) == 0xB69) {
            if (*(unsigned char *)(s0 + 0x31) == 0)
                *(float *)(v + 0x2C) = 0.0f;
            else
                *(float *)(v + 0x2C) = 1.0f;
        }
    }
    func_002B0ED8(s0, s1 + 0x60, 0);
    if (*(float *)(s1 + 0x20) <= 0.0f) {
        func_002AC728(s0, s0 + 0x10, 1.0f, 13.0f, -1);
        func_002A6C58(s0, 0, 3);
        func_0029F338(s0);
    } else if (*(float *)(s0 + 0x18) < D_L14_001B16B0) {
        func_002A6C58(s0, 0, 3);
        func_0029F338(s0);
    }
}
