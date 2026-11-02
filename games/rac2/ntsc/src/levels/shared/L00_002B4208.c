#include "include_asm.h"
typedef unsigned char u8;
typedef float f32;
typedef int V4 __attribute__((mode(TI)));

typedef struct {
    u8 pad0[0x98];
    f32 f98;
    u8 pad1[0x1B4 - 0x9C];
    f32 f1B4;
    u8 pad2[0x324 - 0x1B8];
    f32 f324;
    u8 pad3[0x22C3 - 0x328];
    u8 flag;
} Owner_002B4208;

typedef struct { float x, y, z, w; } __attribute__((aligned(16))) Vec4;
extern Owner_002B4208 D_00189E20;
extern V4 D_00189F10;
extern float func_00283180(float);
extern float func_00283198(float);
extern void func_002830C0(void *, void *, void *);
extern void func_002AF000(void *, float, float, float);
void func_L00_002B4208(float angle)
{
    Vec4 r;
    float v[4];
    float a;
    float s;
    float z;
    Owner_002B4208 *o = &D_00189E20;
    a = o->f98;
    if (angle <= 3.1415927f)
        a = angle;
    s = o->f1B4;
    if (o->flag != 0) {
        z = 0.0f;
        v[0] = func_00283180(z) * s;
        v[1] = func_00283198(z) * s;
        v[2] = z;
        func_002830C0(v, v, o);
        *(V4 *)&r = *(V4 *)v;
    } else {
        func_002AF000(&r, s, a, o->f324);
    }
    D_00189F10 = *(V4 *)&r;
}
