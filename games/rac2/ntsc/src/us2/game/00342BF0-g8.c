#include "include_asm.h"
extern void func_003480D8(void *, int, int);
extern void func_003363C0(void *, float);
extern int func_00336E40(void *);
extern int *func_00336DA8(int, int);
extern char D_001ADC10[], D_001ADC18[];
extern char *D_001A8CFC[] __attribute__((sda));
extern float D_001AE12C __attribute__((sda));
extern void func_003361E0(void *, void *, void *);
extern int *func_00335E20(void *);
extern void func_00336390(void *, void *, int);
extern void func_00347EC8(void *, void *);
extern void func_00341EE8(void *, int);

void func_US2_00342BF0(char *object, void *argument)
{
    int *allocation;
    char *first, *second, *widget;
    *(void **)(object + 4) = argument;
    if (argument) {
        allocation = func_00336DA8(16, func_00336E40(argument));
        *(int **)object = allocation;
        allocation[1] = 0;
        allocation[2] = 0;
        allocation[3] = 0;
        allocation[0] = 0;
    }
    **(float **)object = 250.0f;
    (*(float **)object)[1] = 120.0f;
    first = object + 0xEC;
    second = object + 0x138;
    widget = object + 0x10;
    func_003361E0(first, D_001ADC10, argument);
    func_003361E0(second, D_001ADC18, argument);
    *func_00335E20(first) = 0x60442D00;
    *func_00335E20(second) = 0x55F0C070;
    func_00336390(first, D_001A8CFC[0] + 0x8710, 0x73);
    func_00336390(second, D_001A8CFC[0] + 0x8710, 0x74);
    func_003363C0(first, D_001AE12C);
    func_003363C0(second, D_001AE12C);
    func_00347EC8(widget, argument);
    func_003480D8(widget, 11, 60);
    func_00341EE8(object, 0);
}
