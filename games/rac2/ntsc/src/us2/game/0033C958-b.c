#include "include_asm.h"
extern void func_0033B0A8(void *, float);
extern int func_00336E40(void *);
extern int *func_00336DA8(int, int);
extern void func_0033ABD8(void *, void *, void *);
extern int func_00288FB8(int);
extern void func_0033B050(void *, int, int, int);
extern void func_0033B030(void *, float, float, float, float, float, float);
extern void func_0033C2D8(void *, int);
extern int D_001ADDE0[2] __attribute__((sda));

void func_US2_0033D1F8(char *object, void *argument)
{
    int *allocation;
    char *child;
    int text, second;
    *(void **)object = argument;
    if (argument) {
        allocation = func_00336DA8(16, func_00336E40(argument));
        *(int **)(object + 0x2DC) = allocation;
        allocation[1] = 0;
        allocation[2] = 0;
        allocation[3] = 0;
        allocation[0] = 0;
    }
    **(float **)(object + 0x2DC) = 255.0f;
    (*(float **)(object + 0x2DC))[1] = 195.0f;
    child = object + 8;
    func_0033ABD8(child, argument, D_001ADDE0);
    text = func_00288FB8(0x307A);
    second = func_00288FB8(0x2BE4);
    func_0033B050(child, text, second, func_00288FB8(0x2BE5));
    func_0033B030(child, 0.0f, -143.0f, 0.0f, 114.0f, 0.0f, 140.0f);
    func_0033B0A8(child, 0.65f);
    *(int *)(object + 0x2D8) = 0;
    func_0033C2D8(object, 0);
}
