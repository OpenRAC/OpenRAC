#include "include_asm.h"

/* func_L22_003A3550 is a unit of its own: it needs the opt-in quadword-copy profile and its
   scheduling barrier (-frac2-qcopy -frac2-qcopy-barrier, config/cflags.txt), which give
   retail's grouped `lq`/`sq` with register addresses and keep the instructions around the
   copy on their own side of it. */

typedef int TI128 __attribute__((mode(TI)));

extern char *ref_L22_003A3550_00 __attribute__((sda));

extern int *D_L22_001FE940[];

extern char D_00189EA0[];

extern float func_002F3C40(char *, char *);

extern int func_002A77E0(int);

extern int func_L22_003A48F8(int);

extern void func_L22_003A2988(char *, float);

int func_L22_003A3550(char *arg0, int arg1, float arg2) {
    char *state = *(char **)(ref_L22_003A3550_00 + 0x68);
    char *slots = state + 0x2A0;
    float best = 45.0f;
    int found;
    int w;
    int j;
    int *list;
    char *p;
    int k;

    for (;;) {
        found = 0;
        w = func_002A77E0(0x20);
        list = D_L22_001FE940[*(int *)(slots + (w << 2))];
        p = (char *)list + 0x10;
        for (j = 0; j < *list; j++) {
            if (func_002F3C40(D_00189EA0, p) < best) {
                found = 1;
                break;
            }
            p += 0x10;
        }
        if (found)
            continue;
        if (arg1 == 0)
            continue;
        if (func_L22_003A48F8(w) == 0)
            continue;
        break;
    }
    k = func_002A77E0(*list - 1);
    *(TI128 *)arg0 = *(TI128 *)((char *)list + k * 16 + 0x10);
    func_L22_003A2988(arg0, arg2);
    return 1;
}
