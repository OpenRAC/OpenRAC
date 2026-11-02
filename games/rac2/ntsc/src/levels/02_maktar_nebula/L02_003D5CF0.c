#include "include_asm.h"

/* func_L02_003D5CF0 is a unit of its own: it needs the opt-in quadword-copy profile and its
   scheduling barrier (-frac2-qcopy -frac2-qcopy-barrier, config/cflags.txt), which give retail's grouped `lq`/`sq`
   with register addresses and keep the instructions around the copy on their own side of it. */

typedef int TI128 __attribute__((mode(TI)));

extern void func_00282CF0(void *, void *, void *);

extern float func_00282DF0(void *);

extern void func_00282F20(char *, char *, float);

extern void func_00282CC0(void *, void *, void *);

extern char *func_L02_0033C760(void *, char *, int, float);

char *func_L02_003D5CF0(char *arg0, char *arg1, float step) {
    float v[4];
    float diff[4];
    char *first = 0;
    char *node = 0;
    float len;
    float t;

    func_00282CF0(diff, arg1, arg0);
    len = func_00282DF0(diff);
    func_00282F20((char *)diff, (char *)diff, step);
    *(TI128 *)v = *(TI128 *)arg0;
    for (t = 0.0f; t < len; t += step) {
        node = func_L02_0033C760(v, node, 0, step * 1.3f);
        if (node != 0) {
            if (first == 0) first = node;
        }
        func_00282CC0(v, v, diff);
    }
    return first;
}
