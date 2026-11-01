#include "include_asm.h"

/* func_0032B190 in its own -g unit: the second fade's division carries the
   statement-boundary padding. */

extern unsigned long func_0027C878(int);
extern void func_0027DE50(int, int, int, int, int);
extern float func_00283CE0(int);
extern int func_00283CF0(float);
extern float func_002A7790(float);
extern float func_002A7798(float);
extern void func_0032C2E8(void *);
extern void func_0032BA88(void *);
extern void func_0032B0A8(void *);
extern short D_001A6480[];
extern unsigned long D_001B2810[] __attribute__((sda));

/* Per-frame screen fade of the object: while it is in state 2 (and stays there
   after func_0032C2E8) the white overlay fades in over 60 frames, otherwise it
   fades out after frame 105 over 15 frames. */
void func_0032B190(unsigned char *object)
{
    char *vars = *(char **)(object + 0x68);
    unsigned char state;
    float t;
    int v;

    D_001B2810[0] = func_0027C878(-1);
    state = object[0x20];
    if (state == 2) {
        func_0032C2E8(object);
        if (object[0x20] == state) {
            t = func_00283CE0(*(int *)(vars + 4)) / func_00283CE0(0x3C);
            v = func_00283CF0(func_002A7790(1.0f - t) * 128.0f);
            func_0027DE50(0, D_001A6480[0xA9], 0, D_001A6480[0xA8], (v << 24) | 0xFFFFFF);
            func_0032BA88(object);
            goto done;
        }
    }
    if (*(int *)(vars + 4) >= 0x6A) {
        t = func_00283CE0(*(int *)(vars + 4) - 0x69) / func_00283CE0(0xF);
        v = func_00283CF0(func_002A7798(t) * 128.0f);
        func_0027DE50(0, D_001A6480[0xA9], 0, D_001A6480[0xA8], (v << 24) | 0xFFFFFF);
    }
done:
    func_0032B0A8(object);
}
