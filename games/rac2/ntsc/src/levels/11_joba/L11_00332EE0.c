#include "common.h"

extern void func_00283388(void *m);
extern float func_00283248(float, float);
extern void func_00283410(void *m, void *angles);
extern void func_002830C0(void *dst, void *src, void *m);
extern void func_00283430(void *m, void *angles);
extern void func_00283788(void *dst, void *a, void *b);
extern void func_00283698(void *dst, void *src);
extern void func_00283678(void *dst, void *src);

void func_L11_00332EE0(void *out, float *dir, float roll)
{
    float m0[16];
    float m1[16];
    float m2[16];
    float m3[16];
    float ang[4];
    float d[4] __attribute__((aligned(16)));

    qcopy(d, dir);
    func_00283388(m0);
    ang[2] = -func_00283248(d[0], d[1]);
    ang[0] = 0;
    ang[1] = 0;
    func_00283410(m1, ang);
    func_002830C0(d, d, m1);
    ang[1] = -func_00283248(d[2], d[0]);
    ang[0] = 0;
    ang[2] = 0;
    func_00283430(m2, ang);
    func_002830C0(d, d, m2);
    ang[2] = roll;
    ang[0] = 0;
    ang[1] = 0;
    func_00283430(m3, ang);
    func_00283788(m0, m1, m0);
    func_00283788(m0, m2, m0);
    func_00283788(m0, m3, m0);
    func_00283698(m2, m2);
    func_00283788(m0, m2, m0);
    func_00283698(m1, m1);
    func_00283788(m0, m1, m0);
    qcopy(d, dir);
    func_002830C0(d, d, m0);
    func_00283678(out, m0);
}
