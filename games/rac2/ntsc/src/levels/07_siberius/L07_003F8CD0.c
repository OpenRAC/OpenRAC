#include "include_asm.h"
typedef int Q128 __attribute__((mode(TI)));

extern void func_0029FCC0(void *entity, int a, void *out);
extern void func_00282F20(void *, void *, float);
extern void func_L04_0030C4C0(void *, void *, void *, void *, void *, void *, int, int, int);
extern void func_L07_003F6528(char *);
extern char *func_L07_003F65E8(int);

void func_L07_003F8CD0(char *obj)
{
    char *s2 = *(char **)(obj + 0x68);
    float v10[4];
    float v20[4];
    float v30[4];
    char *p;

    func_0029FCC0(obj, 0, v10);
    *(Q128 *)v30 = 0;
    v30[2] = 5.0f;
    func_00282F20(v20, obj + 0xC0, 2.0f);
    for (;;) {
        p = func_L07_003F65E8(*(int *)(s2 + 0x180));
        if (p == 0) break;
        func_L07_003F6528(p);
        func_L04_0030C4C0(p, v10, v20, p + 0x10, v30, p + 0xF0, 0x5A, 0, 0);
    }
}
