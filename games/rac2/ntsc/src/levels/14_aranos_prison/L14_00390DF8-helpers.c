#include "common.h"

typedef int TI128 __attribute__((mode(TI)));
typedef union { TI128 q; float f[4]; } Vector;

extern char D_0018A0E0[];
extern void func_00282D30(void *, void *, float);
extern void func_00282F20(void *, void *, float);
extern void func_00282DD8(void *, void *, void *);

void func_L14_00390DF8(char *output, char *first, char *second) {
    Vector direction;
    qcopy(&direction, D_0018A0E0);
    func_00282D30(&direction, &direction, -1.0f);
    qcopy(output + 0x30, first);
    qcopy(output, second);
    {
        float one = 1.0f;
        func_00282F20(output, output, one);
        func_00282DD8(output + 0x10, output, &direction);
        func_00282F20(output + 0x10, output + 0x10, one);
        func_00282DD8(output + 0x20, output + 0x10, output);
    }
}
