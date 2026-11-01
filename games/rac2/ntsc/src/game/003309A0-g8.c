/* func_003309A0: -G8 unit. The two globals are read both absolutely and through $gp, so
 * each has an sda name (_1) for the $gp form. The aim vector d is a float[4] copied with
 * qcopy(), which gives retail's frame (a, b, c, d, e, then the 0x30-byte matrix); one float
 * carries the cosine and then the turn, so it lives in $f20 as retail has it. */
#include "common.h"

typedef int TI128 __attribute__((mode(TI)));

extern float D_001B2854;
extern float D_001B2854_1 __attribute__((sda));
extern s32 D_001B2850;
extern s32 D_001B2850_1 __attribute__((sda));
extern char D_001B5570[];

extern float func_00282DF0(void *);
extern void func_00282F20(void *, void *, float);
extern void func_00283098(void *, void *, char *);
extern float func_00282DB0(void *, void *);
extern void func_00282D30(void *, void *, float);
extern void func_00282CF0(void *, void *, void *);
extern void func_00282CC0(void *, void *, char *);
extern void func_00282DD8(void *, void *, void *);
extern float func_002831B0(float);
extern float func_00283198(float);
extern float func_00283180(float);
extern void func_00283958(void *, void *);
extern void func_00283738(void *, void *, void *);

/* Steers the object's heading toward the vector stored in its record: builds the aim vector, measures the angle to the current heading and turns the heading by a tenth of the remainder. */
void func_003309A0(char *obj)
{
    float a[4] __attribute__((aligned(16)));
    float b[4] __attribute__((aligned(16)));
    float c[4] __attribute__((aligned(16)));
    float d[4] __attribute__((aligned(16)));
    float e[4] __attribute__((aligned(16)));
    float f[12] __attribute__((aligned(16)));
    char *rec = *(char **)(obj + 0x68);
    char *dir, *face;
    float cosine;

    a[0] = *(float *)(rec + 0x50);
    a[1] = *(float *)(rec + 0x54);
    D_001B2854 = D_001B2854 * 0.7f;
    a[2] = 0.0f;
    a[3] = 0.0f;
    D_001B2850_1 = 0;
    if (func_00282DF0(a) != 0.0f) {
        dir = obj + 0xE0;
        face = obj + 0xC0;
        func_00282F20(a, a, 1.0f);
        func_00283098(a, a, D_001B5570);
        cosine = func_00282DB0(a, dir);
        func_00282D30(b, dir, cosine);
        func_00282CF0(a, a, b);
        func_00282F20(a, a, 1.0f);
        func_00282F20(c, a, 10.0f);
        func_00282CC0(c, c, obj + 0x10);
        func_00282DD8(d, face, a);
        if (func_00282DF0(d) == 0.0f)
            qcopy(d, face);
        cosine = func_00282DB0(face, a);
        D_001B2854_1 = 1.0f - cosine;
        if (cosine > 0.75f)
            D_001B2850 = 1;
        cosine = 1.5707964f - func_002831B0(cosine);
        cosine = cosine * 0.1f;
        func_00282D30(e, d, func_00283198(cosine));
        e[3] = func_00283180(cosine);
        func_00283958(e, f);
        func_00283738(face, f, face);
    }
}
