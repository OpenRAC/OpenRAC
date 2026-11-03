#include "include_asm.h"

/* func_L02_003DBB70 is a unit of its own: it needs the opt-in quadword-copy profile and its
   scheduling barrier (-frac2-qcopy -frac2-qcopy-barrier, config/cflags.txt), which give retail's grouped `lq`/`sq`
   with register addresses and keep the instructions around the copy on their own side of it. */

typedef int TI128 __attribute__((mode(TI)));

extern char *D_L02_001B359C[] __attribute__((sda));
extern void func_0029FCC0(void *, int, void *);
extern void func_002ACB48(float *, float);
extern float func_002A7940(void);
extern float func_002A7878(float, float);
extern void func_00282D30(void *, void *, float);
extern float func_00283180(float);
extern float func_00283198(float);
extern int func_00283D38(int, int, float);
extern int func_00283CF0(float);
extern void func_002BAA28(void *, void *, void *, int, int, int, int, int, int);

void func_L02_003DBB70(char *arg0, char *arg1) {
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

    for (n = 0; n < 30; n++) {
        func_0029FCC0(D_L02_001B359C[0] + (*(int *)(*(char **)(arg0 + 0x68) + 0x244) << 8), 4, a);
        func_002ACB48(a, 0.5f);
        a[2] += 0.25f;
        angle = func_002A7940();
        r = func_002A7878(3.0f, 16.0f);
        z = func_002A7878(3.0f, 12.0f);
        func_00282D30(b, arg1, func_002A7878(0.0f, 0.0f) * 0.016666668f);
        b[0] += func_00283180(angle) * 0.016666668f * r;
        b[1] += func_00283198(angle) * 0.016666668f * r;
        b[2] += z * 0.016666668f;
        *(TI128 *)c = *(TI128 *)b;
        c[2] -= 0.33333334f;
        for (i = 0; i < 20; i++) {
            float k = func_002A7878(0.5f, 1.5f);

            b[3] = k * 0.25f;
            c[3] = k * 0.125f;
            func_002ACB48(a, 0.25f);
            func_002ACB48(c, 0.016666668f);
            col1 = func_00283D38(0x80FF00EA, 0x80FF00EA, func_002A7878(0.0f, 1.0f));
            col2 = func_00283D38(0x00FF0020, 0x00FF00AA, func_002A7878(0.0f, 1.0f));
            func_002BAA28(a, b, c, col1, col2, 20, 60, func_00283CF0(func_002A7878(10.0f, 50.0f)), -1);
        }
    }
}
