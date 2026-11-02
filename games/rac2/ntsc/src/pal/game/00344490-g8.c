#include "include_asm.h"
extern float D_001AE188 __attribute__((sda));
extern float D_001AE18C;
extern float D_001AE190 __attribute__((sda));
extern float D_001AE194 __attribute__((sda));
extern void func_003480F0(void *, float, float);
extern void func_003480D8(void *, int, int);
extern void *func_00347F80(void *, int);
extern int func_00348128(void *);

void func_PAL_00344490(char *self, int arg)
{
    char *child = self + 0x188;
    float *position = *(float **)(self + 0x180);
    func_003480F0(child, position[0] + D_001AE188, position[1] + D_001AE18C);
    func_003480D8(child, (int)D_001AE190, (int)D_001AE194);
    func_00347F80(child, arg);
    *(int *)(self + 0x16C) = func_00348128(child);
}

extern void func_00335E38(void *, float, float, float, float);
extern float D_001AE198 __attribute__((sda));
extern float D_001AE19C;
extern float D_001AE1A0 __attribute__((sda));
extern float D_001AE1A4;
extern float D_001AE1A8[2] __attribute__((sda));
extern int func_00348130(void *);
extern void func_003363C0(void *, float);
extern void func_00342420(void *);

int func_PAL_00344528(char *self, int arg)
{
    char *child = self + 0x4C;
    float *position = *(float **)(self + 0x180);
    float *scale;
    unsigned int index;
    func_00335E38(self, D_001AE198 + position[0], D_001AE19C + position[1], 0.0f, 0.0f);
    position = *(float **)(self + 0x180);
    func_00335E38(child, D_001AE1A0 + position[0], D_001AE1A4 + position[1], 0.0f, 0.0f);
    index = func_00348130(self + 0x188) - 1;
    scale = (float *)((char *)D_001AE1A8 + (index << 2));
    func_003363C0(self, *scale);
    func_003363C0(child, *scale);
    func_00342428(self, arg);
    func_003423C8(self);
    func_00342420(self);
    return *(int *)(self + 0x16C);
}
