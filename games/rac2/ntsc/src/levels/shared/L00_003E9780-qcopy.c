#include "include_asm.h"
typedef int CurveWord128 __attribute__((mode(TI)));
typedef union { CurveWord128 word; float values[4]; } CurveVector;
extern float func_00283CE0(int);
extern void func_00282CC0(void *, void *, void *);
extern void func_L00_003E9490(void *, void *, void *, float);
extern void func_L00_003E9540(void *, void *, void *, void *, void *, float);
extern float func_002A7878(float, float);
extern void func_00282F20(void *, void *, float);
extern void func_L00_003E9B90(void *, int, void *, int);

extern void ref_L00_003E9780_00(void *, void *, void *, void *, void *, float);
void func_L00_003E9780(char *object, char *a1, char *a2, char *a3) {
    CurveVector points[11];
    CurveVector start, end, jitter;
    int i;
    float t;
    CurveWord128 *p0 = (CurveWord128 *)(a3 + 0x50);
    CurveWord128 *p1 = (CurveWord128 *)(a3 + 0x60);
    for (i = 0; i < 10; i++) {
        t = func_00283CE0(i) / func_00283CE0(9);
        func_L00_003E9490(&points[i], a1, a2, t);
        start.word = *p0;
        end.word = *p1;
        func_00282CC0(&start, &start, a1);
        func_00282CC0(&end, &end, a2);
        ref_L00_003E9780_00(&points[i], a1, a2, &start, &end, t);
        jitter.values[3] = 0;
        jitter.values[0] = func_002A7878(-1.0f, 1.0f);
        jitter.values[1] = func_002A7878(-1.0f, 1.0f);
        jitter.values[2] = func_002A7878(-1.0f, 1.0f);
        func_00282F20(&jitter, &jitter, 0.04f);
        func_00282CC0(&points[i], &points[i], &jitter);
    }
    func_L00_003E9B90(object, 0x80, points, 11);
}
