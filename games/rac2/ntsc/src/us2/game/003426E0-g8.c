#include "include_asm.h"
extern float D_001AE0E8 __attribute__((sda));
extern float D_001AE0EC;
extern float D_001AE0F0 __attribute__((sda));
extern float D_001AE0F4 __attribute__((sda));
extern void func_003480F0(void *, float, float);
extern void func_003480D8(void *, int, int);
extern void *func_00347F80(void *, int);
extern int func_00348128(void *);
extern int func_002A9588(int, int, int, int, int);

void func_US2_003426E0(char *self, int arg)
{
    char *indices = self + 0x1B8;
    char *child = self + 0x238;
    int old;
    float *position = *(float **)(self + 0x228);
    func_003480F0(child, position[0] + D_001AE0E8, position[1] + D_001AE0EC);
    func_003480D8(child, (int)D_001AE0F0, (int)D_001AE0F4);
    func_00347F80(child, arg);
    old = *(int *)(indices + (*(int *)(self + 0x31C) << 2));
    *(int *)(indices + (*(int *)(self + 0x31C) << 2)) = func_00348128(child);
    indices += *(int *)(self + 0x31C) << 2;
    if (old != *(int *)indices)
        func_002A9588(0, 0, 1, 0, 1);
}

typedef float f32_Fd368add8;
extern f32_Fd368add8 D_L01_001AE3F8 __attribute__((sda));
extern f32_Fd368add8 D_L01_001AE408 __attribute__((sda));
extern f32_Fd368add8 D_L01_001AE400 __attribute__((sda));
extern f32_Fd368add8 D_L01_001AE410 __attribute__((sda));
extern f32_Fd368add8 D_L01_001AE418 __attribute__((sda));
extern f32_Fd368add8 D_L01_001AE3FC;
extern f32_Fd368add8 D_L01_001AE40C;
extern f32_Fd368add8 D_L01_001AE404;
extern f32_Fd368add8 D_L01_001AE414;
extern f32_Fd368add8 D_L01_001AE41C;
extern void func_00335E38_1(void *a0, f32_Fd368add8 a1, f32_Fd368add8 a2, f32_Fd368add8 a3, f32_Fd368add8 a4);
extern void *func_00335E20_1(void *a0);
extern int func_002A9588(int a0, int a1, int a2, int a3, int a4);
extern void func_003417D0(void *a0, int a1);
extern void func_00341760(void *a0);
extern void ref_L00_003418C0_00(void *a0);

int func_US2_003427D0(char *self, int arg1)
{
    f32_Fd368add8 *v;

    v = *(f32_Fd368add8 **)(self + 552);
    func_00335E38_1(self, D_L01_001AE3F8 + v[0], D_L01_001AE3FC + v[1], 0.0f, 0.0f);
    v = *(f32_Fd368add8 **)(self + 552);
    func_00335E38_1(self + 152, D_L01_001AE408 + v[0], D_L01_001AE40C + v[1], 0.0f, 0.0f);
    v = *(f32_Fd368add8 **)(self + 552);
    func_00335E38_1(self + 76, D_L01_001AE400 + v[0], D_L01_001AE404 + v[1], 0.0f, 0.0f);
    v = *(f32_Fd368add8 **)(self + 552);
    func_00335E38_1(self + 228, D_L01_001AE410 + v[0], D_L01_001AE414 + v[1], 0.0f, 0.0f);
    v = *(f32_Fd368add8 **)(self + 552);
    func_00335E38_1(self + 464, D_L01_001AE418 + v[0], D_L01_001AE41C + v[1], 0.0f, 0.0f);

    *(int *)func_00335E20_1(self + 304) = func_002A9588(0x60442D00, 0x70FFFEED, 20, 0, 0);
    func_003417D0(self, arg1);
    func_00341760(self);
    ref_L00_003418C0_00(self);
    return *(int *)(self + (*(int *)(self + 796) << 2) + 440);
}
