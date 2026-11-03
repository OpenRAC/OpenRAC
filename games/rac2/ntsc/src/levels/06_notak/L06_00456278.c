#include "include_asm.h"

/* Emit the collision burst and six fragments, then retire the triggering object. */
extern void func_002ABBB8(char *, char *, int, float, float, int, int, int,
                          float, float, float, float, int, float, int, int, int, int, int);
extern void func_002A7A00(float *, float, float);
extern float func_00282C48(float);
extern void func_002B0A68(char *, int, float *, float, float, float, float, int);

extern char *func_002A9640(char *, int, int);
extern int func_002E5CF8(int, int, char *);
extern void func_0029F338(char *);
extern void func_00282CC0(char *, char *, char *);
extern char D_L06_001A8F20[] __attribute__((sda));
void func_L06_00456278(char *obj)
{
    float v[4];
    char *hit = (char *)func_002A9640(obj, 0x4010000, 0);
    if (hit && *(short *)(*(char **)(hit + 0x20) + 0xAA) != 0xFE5) {
        func_002E5CF8(0, 0, obj);
        func_002ABBB8(obj, obj + 0x10, 0, 0.0f, 0.0f, 10, 3, 9,
                      4.0f, 2.0f, 9.0f, 0.0f, -1, 15.0f, 1, 1, -1, 0, 0x810001);
        func_002A7A00(v, 0.06666667014360428f, 0.10000000894069672f);
        v[2] = func_00282C48(v[2]) * 1.5f;
        func_00282CC0((char *)v, (char *)v, D_L06_001A8F20);
        func_002B0A68(obj, 1, v, 3.0f, 6.0f, 3.0f, 6.0f, 255);
        func_002A7A00(v, 0.06666667014360428f, 0.10000000894069672f);
        v[2] = func_00282C48(v[2]) * 1.5f;
        func_00282CC0((char *)v, (char *)v, D_L06_001A8F20);
        func_002B0A68(obj, 4, v, 3.0f, 6.0f, 3.0f, 6.0f, 255);
        func_002A7A00(v, 0.06666667014360428f, 0.10000000894069672f);
        v[2] = func_00282C48(v[2]) * 1.5f;
        func_00282CC0((char *)v, (char *)v, D_L06_001A8F20);
        func_002B0A68(obj, 8, v, 3.0f, 6.0f, 3.0f, 6.0f, 255);
        func_002A7A00(v, 0.06666667014360428f, 0.10000000894069672f);
        v[2] = func_00282C48(v[2]) * 1.5f;
        func_00282CC0((char *)v, (char *)v, D_L06_001A8F20);
        func_002B0A68(obj, 16, v, 3.0f, 6.0f, 3.0f, 6.0f, 255);
        func_002A7A00(v, 0.06666667014360428f, 0.10000000894069672f);
        v[2] = func_00282C48(v[2]) * 1.5f;
        func_00282CC0((char *)v, (char *)v, D_L06_001A8F20);
        func_002B0A68(obj, 32, v, 3.0f, 6.0f, 3.0f, 6.0f, 255);
        func_002A7A00(v, 0.06666667014360428f, 0.10000000894069672f);
        v[2] = func_00282C48(v[2]) * 1.5f;
        func_00282CC0((char *)v, (char *)v, D_L06_001A8F20);
        func_002B0A68(obj, 64, v, 3.0f, 6.0f, 3.0f, 6.0f, 255);
        func_0029F338(obj);
    }
    *(unsigned char *)(obj + 0xA8) = 255;
}
