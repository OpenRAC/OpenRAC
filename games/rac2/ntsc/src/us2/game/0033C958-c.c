#include "include_asm.h"
extern void func_0033B0A8(void *, float);
extern int func_00336E40(void *);
extern int *func_00336DA8(int, int);
extern void func_0033ABD8(void *, void *, void *);
extern int func_00288FB8(int);
extern void func_0033B050(void *, int, int, int);
extern void func_0033B030(void *, float, float, float, float, float, float);
extern void func_0033D028(void *, int);
extern int D_001ADE48[2] __attribute__((sda));
extern char D_001ADC88[];
extern void func_00336408(void *, int, int, void *, void *);
extern void func_00336CC8(void *, int);
extern void func_00336B90(void *, int);
extern void func_00336B98(void *, int);
extern void func_00336498(void *, int, int);
extern void func_003364B0(void *, int, int);
extern void func_00336CE0(void *, int);
extern void func_00336CC0(void *, int);

void func_US2_0033DE10(char *object, void *argument)
{
    int *allocation;
    char *child, *widget, *secondWidget;
    void *table;
    int text, second;
    *(void **)object = argument;
    if (argument) {
        allocation = func_00336DA8(16, func_00336E40(argument));
        *(int **)(object + 0x36C) = allocation;
        allocation[1] = 0;
        allocation[2] = 0;
        allocation[3] = 0;
        allocation[0] = 0;
    }
    **(float **)(object + 0x36C) = 255.0f;
    (*(float **)(object + 0x36C))[1] = 195.0f;
    child = object + 8;
    widget = object + 0x2DC;
    secondWidget = object + 0x324;
    func_0033ABD8(child, argument, D_001ADE48);
    text = func_00288FB8(0x2C2D);
    second = func_00288FB8(0x2BE4);
    func_0033B050(child, text, second, func_00288FB8(0x2BE5));
    func_0033B030(child, 0.0f, -143.0f, 0.0f, 114.0f, 0.0f, 140.0f);
    func_0033B0A8(child, 0.67f);
    table = D_001ADC88;
    func_00336408(widget, 150, 1, table, argument);
    func_00336CC8(widget, 16);
    func_00336B90(widget, 0x400);
    func_00336B98(widget, 0);
    func_00336498(widget, 0x55F0C070, 0x55F0C070);
    func_003364B0(widget, 0x60442D00, 0x60442D00);
    func_00336CE0(widget, 0x20FFFFFF);
    func_00336CC0(widget, 0);
    func_00336408(secondWidget, 150, 1, table, argument);
    func_00336CC8(secondWidget, 16);
    func_00336B90(secondWidget, 0x400);
    func_00336B98(secondWidget, 0);
    func_00336498(secondWidget, 0x55F0C070, 0x55F0C070);
    func_003364B0(secondWidget, 0x60442D00, 0x60442D00);
    func_00336CE0(secondWidget, 0x20FFFFFF);
    func_00336CC0(secondWidget, 0);
    *(int *)(object + 0x2D8) = 0;
    func_0033D028(object, 0);
}
