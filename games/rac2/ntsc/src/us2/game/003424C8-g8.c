#include "include_asm.h"
extern int D_001AE0C8 __attribute__((sda));
extern char D_002652F0[];
extern void func_00348118(void *, int);
extern void func_00348068(unsigned char *, float *);
extern int func_00288FB8(int);
extern void func_00336A38(void *, unsigned int);
extern void func_003363C0(void *, float);

void func_US2_00342520(char *self, int idx)
{
    float *records;
    float f;
    unsigned char *child;
    if ((unsigned int)idx > 2) return;
    *(int *)(self + 0x1C4) = ((int *)&D_001AE0C8)[idx];
    *(int *)(self + 0x1C8) = (int)(D_002652F0 + idx * 0x20);
    *(int *)(self + 0x1CC) = (idx == 0) ? 7 : ((idx == 1) ? 8 : 6);
    if (idx != 0) {
        if (idx == 2) f = 2.0f;
        else f = 1.0f;
    } else {
        f = 0.0f;
    }
    child = (unsigned char *)self + 0x238;
    {
        int *p = (int *)(self + idx * 4);
        func_00348118(child, p[0x6E]);
    }
    records = (idx == 0) ? *(float **)(self + 0x310) : ((idx == 1) ? *(float **)(self + 0x314) : *(float **)(self + 0x318));
    func_00348068(child, records);
    func_00336A38(self + 0x1D0, func_00288FB8((idx == 0) ? 0x2BEB : ((idx == 1) ? 0x2BEC : 0x3094)));
    func_003363C0(self, f);
    func_003363C0(self + 0x4C, f);
    func_003363C0(self + 0xE4, f);
    *(int *)(self + 0x31C) = idx;
}

extern float D_001AE0E0 __attribute__((sda));
extern float D_001AE0DC;
extern float D_001AE0D8 __attribute__((sda));
extern void func_00335E38(void *, float, float, float, float);

void func_US2_00342670(char *self)
{
    int index = *(int *)(self + 0x31C);
    float *v = *(float **)(self + 0x228);
    int *base = (int *)(self + (index << 2));

    func_00335E38(self + 0x130, D_001AE0D8 + v[0],
            (D_001AE0DC + D_001AE0E0 * (float)base[0x6E]) + v[1],
            0.0f, 0.0f);
}
