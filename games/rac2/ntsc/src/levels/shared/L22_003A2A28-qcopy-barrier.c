#include "include_asm.h"


typedef int s32;
typedef short s16;
typedef int TI128 __attribute__((mode(TI)));
extern void func_00282CF0(float *, const float *, const float *);
extern void func_00282F20(char *, char *, float);
extern void func_00282CC0(char *, char *, char *);
extern float func_00282E48(char *, char *);
extern char D_00189EA0[];
extern int *D_L23_001FF300[];

/* Same body as func_L23_00418DC0, with this function's own callees. */
void func_L22_003A2A28(char *arg0, s32 arg1) {
    extern void func_L22_003A2E10();
    struct {
        TI128 a;
        float b[4];
    } loc;
    char *src = arg0 + 0x10;
    char *state;
    char *slots;
    float best;
    s32 i;

    state = *(char **)(arg0 + 0x68);
    loc.a = *(TI128 *)src;
    func_00282CF0(loc.b, (float *)src, (float *)D_00189EA0);
    slots = state + 0x2A0;
    best = 30000.0f;
    func_00282F20((char *)loc.b, (char *)loc.b, 4.0f);
    func_00282CC0((char *)&loc.a, (char *)&loc.a, (char *)loc.b);
    for (i = 0; i < 0x20; i++) {
        int *obj;
        s32 k;

        if (arg1 != 0 && func_L22_003A48F8(i) == 0)
            continue;
        obj = D_L23_001FF300[*(int *)(slots + (i << 2))];
        for (k = 0; k < *obj; k++) {
            float d = func_00282E48((char *)obj + 0x10 + k * 0x10, (char *)&loc.a);

            if (d < best) {
                best = d;
                func_L22_003A2E10(arg0, i);
                *(s16 *)(state + 0x356) = k;
            }
        }
    }
}
