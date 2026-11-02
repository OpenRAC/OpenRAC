#include "include_asm.h"

extern char *D_L02_001FF1C0[];
extern void func_00282C88(void *);
extern float func_00282E80(void *, void *);
extern void func_00282CC0(void *, void *, void *);
extern void func_00282D30(void *, void *, float);
extern float func_00282E48(void *, void *);
extern int func_00283CF0(float);

/* Prepares the actor's path (built with -g: the line labels pad the two divisions with nops).
   The table entry the state names holds a count and 16-byte points from +0x10: the fourth float
   of each point but the last becomes the distance to the next one, the points are summed and
   scaled by 1/(count - 1) into the centre at +0x20, the largest func_00282E48 distance from the
   centre plus the margin at +4 becomes the radius at +0x2C, and two ratios of the entry's float
   at +0x1C become the words at +0x3C and +0x40. The guarded do-while with a second pointer walked beside
   the point index is the loop shape retail has. */
void func_L02_003EEAD8(char *arg0) {
    char *state = *(char **)(arg0 + 0x68);
    char *entry = D_L02_001FF1C0[*(int *)state];
    float sum[4];
    int i;
    int j;

    func_00282C88(sum);
    i = 0;
    if (*(int *)entry - 1 > 0) {
        char *q = entry + 0x20;

        do {
            char *p = entry + i * 16 + 0x10;

            *(float *)(p + 0xC) = func_00282E80(p, q);
            func_00282CC0(sum, sum, p);
            q += 0x10;
            i++;
        } while (i < *(int *)entry - 1);
    }
    func_00282D30(state + 0x20, sum, 1.0f / (float)(*(int *)entry - 1));
    *(float *)(state + 0x2C) = 0.0f;
    for (j = 0; j < *(int *)entry; j++) {
        float f = func_00282E48(entry + j * 16 + 0x10, state + 0x20);

        if (*(float *)(state + 0x2C) < f)
            *(float *)(state + 0x2C) = f;
    }
    *(float *)(state + 0x2C) += *(float *)(state + 4);
    *(int *)(state + 0x3C) = func_00283CF0(*(float *)(state + 8) / *(float *)(entry + 0x1C));
    *(int *)(state + 0x40) = func_00283CF0(*(float *)(state + 4) / *(float *)(entry + 0x1C));
}
