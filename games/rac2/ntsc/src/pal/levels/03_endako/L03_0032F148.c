#include "include_asm.h"

#include "common.h"
extern void *func_L00_0032C638(char *a0);
extern float func_00283248(float a, float b);
extern float func_00283B98(float a, float b);
extern int func_00282978(short *p);
extern int func_002A7820(int a0, int a1);
extern int func_L02_00331440(char *a0, char *a1, float f12, char *a2, float f13, float f14);

/* Aim the record at the shape the caller found. */
int func_PAL_L03_0032F148(char *self, float *target, char *third, float amount, float f13, float f14)
{
    char *data = (char *)func_L00_0032C638(self);
    float angle;

    if (data == 0)
        return 8;
    angle = func_00283248(target[0] - *(float *)(self + 0x10),
                         target[1] - *(float *)(self + 0x14));
    angle = func_00283B98(angle, amount);
    if (*(float *)(data + 0xDC) != 0.0f) {
        angle = func_00283B98(angle, *(float *)(data + 0xDC));
        if (func_00282978((short *)(data + 0xE0))) {
            *(float *)(data + 0xDC) = -*(float *)(data + 0xDC);
            *(short *)(data + 0xE0) = func_002A7820(*(short *)(data + 0xE2), *(short *)(data + 0xE4));
        }
    }
    return func_L02_00331440(self, target, angle, third, f13, f14);
}

