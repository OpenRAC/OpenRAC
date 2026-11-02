#include "include_asm.h"

typedef int TI128 __attribute__((mode(TI)));

extern void func_002ACB48(float *, float);
extern float func_002A7940(void);
extern float func_002A7878(float, float);
extern void func_00282D30(void *, void *, float);
extern float func_00283180(float);
extern float func_00283198(float);
extern int func_00283D38(int, int, float);
extern int func_00283CF0(float);
extern void func_002BAA28(void *, void *, void *, int, int, int, int, int, int);

void func_L08_00422578(char *arg0, char *arg1) {
    float a[4];
    float b[4];
    float c[4];
    int n;
    int i;
    float angle;
    float r;
    float z;
    int col1;
    int col2;
    char *src = arg0 + 0x10;

    for (n = 0; n < 20; n++) {
        *(TI128 *)a = *(TI128 *)src;
        func_002ACB48(a, 0.25f);
        a[2] += 0.25f;
        angle = func_002A7940();
        r = func_002A7878(1.0f, 8.0f);
        z = func_002A7878(2.0f, 6.0f);
        func_00282D30(b, arg1, func_002A7878(0.0f, 0.0f) * 0.016666668f);
        b[0] += func_00283180(angle) * 0.016666668f * r;
        b[1] += func_00283198(angle) * 0.016666668f * r;
        b[2] += z * 0.016666668f;
        *(TI128 *)c = *(TI128 *)b;
        c[2] -= 0.16666667f;
        for (i = 0; i < 10; i++) {
            float k = func_002A7878(0.5f, 1.5f);

            b[3] = k * 0.125f;
            c[3] = k * 0.065f;
            func_002ACB48(a, 0.125f);
            func_002ACB48(c, 0.0083333338f);
            col1 = func_00283D38(0x8000EEEE, 0x8000FF90, func_002A7878(0.0f, 1.0f));
            col2 = func_00283D38(0xFFEE, 0xFFEE, func_002A7878(0.0f, 1.0f));
            func_002BAA28(a, b, c, col1, col2, 10, 30, func_00283CF0(func_002A7878(5.0f, 25.0f)), -1);
        }
    }
}
