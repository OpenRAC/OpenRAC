#include "include_asm.h"

extern int func_00282978(unsigned short *);
extern int func_002A77E0(int);
extern float func_00283B98(float, float);
extern void func_002AEE70(unsigned char *, unsigned char *, unsigned char *, float, float, float);
extern void func_002AEF28(unsigned char *, unsigned char *, unsigned char *, float, float, float);
extern float func_002AA3A8(unsigned char *, unsigned char *, float, float, float, float);

/* Feeds the sub-object two measured channel ramps, then, only in state 1 and
   only while the sub-object's halfword 0x3D4 is live, stamps it 0x104 and
   folds a 2.0943954 radian turn into its 0xF8 angle -- sign drawn from the
   resident two-way draw -- before the shared four-float ramp body runs on the
   block at 0x3D8. */
void func_PAL_L09_00402A88(unsigned char *a0) {
    unsigned char *s1 = a0;
    unsigned char *s0 = *(unsigned char **)(s1 + 0x68);
    func_002AEE70(a0, s0 + 0x3C0, s0 + 0x3C4, 0.25f, 0.06f, -1.0f);
    func_002AEF28(s1, s0 + 0x3C8, s0 + 0x3CC, 0.15f, 0.05f, 0.025f);
    if (s1[0x20] != 1)
        return;
    if (func_00282978((unsigned short *)(s0 + 0x3D4)) != 0) {
        float t = 2.0943954f;
        *(short *)(s0 + 0x3D4) = 0xD9;
        if (func_002A77E0(2) != 0)
            t = -t;
        *(float *)(s0 + 0x3D0) = func_00283B98(*(float *)(s1 + 0xF8), t);
    }
    func_002AA3A8(s1 + 0xF8, s0 + 0x3D8, *(float *)(s0 + 0x3D0), 0.003f, 0.2f,
                  0.02f);
}

