#include "include_asm.h"

extern char D_00189E20[];

extern void func_00282D30(char *dst, char *src, float k);

extern void func_00282CC0(void *out, void *in, void *m);

extern float func_002A89A8(char *a, int b, float f);

extern float func_00282C48(float);

extern void func_L00_002D40A0(char *a, int b, float f);

#ifndef RAC2_T_V4_5098
#define RAC2_T_V4_5098
typedef int V4_5098 __attribute__((mode(TI)));
#endif

/* Builds a 16-byte probe at a stack local from the shared block and its +0x80 matrix,
   raises the third word by 0.5f, calls the level's float selector with that same 0.5f and
   stores its answer back into the third word, then compares the block's +0x88 against the
   probe: when the difference is not greater than 0.3f the level's own setter runs and the
   probe is copied over the block's +0x80. */
void func_L00_002D5098(void)
{
    char *base = D_00189E20;
    char *dst = base + 0x80;
    V4_5098 local;
    float x;

    func_00282D30((char *)&local, base, -0.75f);
    func_00282CC0(&local, &local, dst);
    *(float *)((char *)&local + 8) = *(float *)((char *)&local + 8) + 0.5f;
    *(float *)((char *)&local + 8) = func_002A89A8((char *)&local, 0, 0.5f);
    x = func_00282C48(*(float *)(base + 0x88) - *(float *)((char *)&local + 8));
    if (0.3f < x)
        return;
    func_L00_002D40A0(dst, 1, *(float *)(base + 0x98));
    *(V4_5098 *)dst = local;
}
