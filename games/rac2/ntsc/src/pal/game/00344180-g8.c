#include "include_asm.h"
extern void func_003363C0(void *, float);
extern int func_00336E40(void *);
extern int *func_00336DA8(int, int);
extern char D_001ADC10[], D_001ADC18[];
extern char *D_001A8CFC[] __attribute__((sda));
extern void func_003361E0(void *, void *, void *);
extern int *func_00335E20(void *);
extern void func_00336390(void *, void *, int);
extern void func_00347EC8(void *, void *);
extern char D_001ADC20[], D_001ADFC0[], D_001AE0C0[];
extern int D_001AE138 __attribute__((sda));
extern void func_00336710(void *, void *, void *);
extern void func_00336920(void *, int, int);
extern void func_00115484(void *, int, int);
extern void func_00348118(void *, int);
extern void func_00335F88(void *, float, float, float, float);

/* Sets up a level object: it allocates the pair of positions at +0x180 and puts
   253.0 and 200.0 in it, names the four children at +0x00, +0x4C, +0xE4 and
   +0x130, gives each of them its colour and its metric, and ends with the
   object's own fields at +0x170, +0x174, +0x178 and +0x17C. */
void func_PAL_00344180(char *object, void *arg)
{
    int *allocation;
    *(void **)(object + 0x184) = arg;
    if (arg != 0) {
        allocation = func_00336DA8(16, func_00336E40(arg));
        *(int **)(object + 0x180) = allocation;
        allocation[1] = 0;
        allocation[2] = 0;
        allocation[3] = 0;
        allocation[0] = 0;
    }
    *(int *)(object + 0x170) = 1;
    (*(float **)(object + 0x180))[0] = 253.0f;
    (*(float **)(object + 0x180))[1] = 200.0f;
    func_003361E0(object, D_001ADC10, arg);
    func_003361E0(object + 0x4C, D_001ADC20, arg);
    func_003361E0(object + 0xE4, D_001ADFC0, arg);
    func_00336710(object + 0x130, D_001AE0C0, arg);
    func_00347EC8(object + 0x188, arg);
    *func_00335E20(object) = 0x60442D00;
    *func_00335E20(object + 0x4C) = 0x55F0C070;
    *func_00335E20(object + 0xE4) = 0x70FFFEED;
    *func_00335E20(object + 0x130) = 0x60F0F0B0;
    func_003363C0(object, 1.0f);
    func_003363C0(object + 0x4C, 1.0f);
    func_00336390(object, D_001A8CFC[0] + 0x8710, 0xCD);
    func_00336390(object + 0x4C, D_001A8CFC[0] + 0x8710, 0xCE);
    func_00336390(object + 0xE4, D_001A8CFC[0] + 0x8710, 0x95);
    func_00335F88(object + 0xE4, 1.0f, 1.05f, 0.0f, 0.0f);
    func_00336920(object + 0x130, 0xEA9D, 0);
    func_00115484(object + 0x16C, 0, 4);
    func_00335F88(object + 0x130, 32.0f, 32.0f, 0.0f, 0.0f);
    *(int *)(object + 0x174) = 0xEAA6;
    *(void **)(object + 0x178) = &D_001AE138;
    *(int *)(object + 0x17C) = 4;
    func_00348118(object + 0x188, *(int *)(object + 0x16C));
}
