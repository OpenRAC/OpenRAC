#include "include_asm.h"

extern float func_00283CE0(int a);
extern int func_00283CF0_2(float f);
extern int func_00283D38_1(float, int, int);
extern float func_002A7798(float f);
extern float func_002A7790(float f);
extern void func_00282D30_13(char *a, char *b, float f);
extern void func_00282CC0_13(char *a, char *b, char *c);
extern void func_00300190_1(char *a, char *b);
extern int func_00282978_3(char *p);
extern void func_002B9180_6(char *p);

/* Hand-adapted from func_002C8880. */
void func_L00_003406E8(char *self) {
    float a[4];
    float b[4];
    char *p = self + 0x20;
    float t, f20, f21, f22;
    int color;

    b[0] = *(float *)(self + 0x20);
    b[1] = *(float *)(p + 4);
    b[2] = *(float *)(p + 8);
    b[3] = 0;
    func_00300190_1(self + 0x10, self + 0x10);
    t = func_00283CE0(*(short *)(self + 0xA)) / func_00283CE0(*(unsigned char *)(p + 0x11));
    f22 = 1.0f - t;
    if (*(unsigned char *)(p + 0x13) == 0) {
        color = func_00283D38_1(f22, 0x103080, *(int *)(p + 0xC));
        *(int *)(self + 4) = color | (*(unsigned char *)(p + 0x12) << 24);
        *(float *)(self + 0xC) = *(float *)(p + 0x14) + (*(float *)(p + 0x18) - *(float *)(p + 0x14)) * f22;
        func_00282D30_13(a, b, *(float *)(p + 0x1C) * func_002A7798(f22));
    } else {
        f21 = func_00283CE0(*(unsigned char *)(p + 0x12));
        f20 = func_00283CE0(*(unsigned char *)(p + 0x12));
        f20 = f20 * func_002A7790(f22);
        *(int *)(self + 4) = *(int *)(p + 0xC) | (func_00283CF0_2(f21 - f20) << 24);
        func_00282D30_13(a, b, *(float *)(p + 0x1C));
    }
    func_00282CC0_13(self + 0x10, a, self + 0x10);
    *(unsigned char *)(self + 8) = *(unsigned char *)(self + 8) + *(unsigned char *)(p + 0x10);
    if (func_00282978_3(self + 0xA) != 0) {
        if (*(unsigned char *)(p + 0x13) == 0) {
            *(unsigned char *)(p + 0x13) = 1;
            *(unsigned char *)(p + 0x11) = 0x14;
            *(short *)(self + 0xA) = 0x14;
        } else {
            func_002B9180_6(self);
        }
    }
}
