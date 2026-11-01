#include "include_asm.h"

typedef int s32;
typedef float f32;
typedef unsigned char u8;

/* The 128-bit unit the group copy needs. */
typedef int V4TI __attribute__((mode(TI)));

extern float *func_002AB1F0(char *);

extern void func_00282D30(void *, void *, f32);

extern void func_00282CC0(void *, void *, void *);

extern void func_00282CF0(void *, void *, void *);

extern void func_00282F20(void *, void *, f32);

/* Fetches the record the caller names, then either builds a vector from the
   object's own fields or copies the object's second vector, and hands the
   result to the transform and the closing helper. */
void func_L00_0032A1A8(char *a0, char *a1, char *a2, f32 t)
{
    char buf[32];
    float *r;

    r = func_002AB1F0(a0);
    if (r != 0) {
        func_00282D30(buf + 0x10, a0 + 0xE0, *(f32 *)((char *)r + 0x10));
        func_00282CC0(buf, a0 + 0x10, buf + 0x10);
    } else {
        *(V4TI *)buf = *(V4TI *)(a0 + 0x10);
    }
    func_00282CF0(a2, buf, a1);
    func_00282F20(a2, a2, t);
}
